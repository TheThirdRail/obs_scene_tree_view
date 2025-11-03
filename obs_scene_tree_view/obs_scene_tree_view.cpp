#include "obs_scene_tree_view/obs_scene_tree_view.h"

#include "obs_scene_tree_view/version.h"

#include <QLineEdit>
#include <QAction>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidgetAction>

#include <obs-module.h>
#include <util/platform.h>
#include <obs-frontend-api.h>


OBS_DECLARE_MODULE();
OBS_MODULE_AUTHOR("DigitOtter");
OBS_MODULE_USE_DEFAULT_LOCALE(PROJECT_DATA_FOLDER, "en-US");

// Global dock pointer and registration status for retry logic
static ObsSceneTreeView *g_stv_dock = nullptr;
static bool g_stv_added = false;


// OBS wrapper-equivalent: sets dynamic "class" and forces stylesheet recalculation
static inline void setClasses(QWidget *widget, const QString &newClasses)
{
	if (!widget)
		return;
	if (widget->property("class").toString() != newClasses) {
		widget->setProperty("class", newClasses);
		/* force style sheet recalculation */
		QString qss = widget->styleSheet();
		widget->setStyleSheet("/* */");
		widget->setStyleSheet(qss);
	}
}

// Ensure disabled icons look identical to enabled by duplicating Normal pixmaps into Disabled
static QIcon NonDimmedDisabled(const QIcon &src)
{
	if (src.isNull())
		return src;
	QIcon out;
	const QList<QSize> sizes = src.availableSizes(QIcon::Normal);
	if (sizes.isEmpty())
		return src;
	for (const QSize &sz : sizes) {
		QPixmap pm = src.pixmap(sz, QIcon::Normal);
		out.addPixmap(pm, QIcon::Normal);
		out.addPixmap(pm, QIcon::Active);
		out.addPixmap(pm, QIcon::Disabled);
	}
	return out;
}




MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("SceneTreeView");
}

MODULE_EXPORT bool obs_module_load(void)
{
	blog(LOG_INFO, "[%s] loaded version %s", obs_module_name(), PROJECT_VERSION);

	BPtr<char> stv_config_path = obs_module_config_path("");
	if(!os_mkdir(stv_config_path))
		blog(LOG_WARNING, "[%s] failed to create config dir '%s'", obs_module_name(), stv_config_path.Get());

	QMainWindow *main_window = reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);
	ObsSceneTreeView *dock = new ObsSceneTreeView(main_window);
	dock->setObjectName("obs_scene_tree_view");
	{
		const char *t = obs_module_text("SceneTreeView.Title");
		QString title = QString::fromUtf8(t ? t : "");
		if (title.isEmpty() || title == QLatin1String("SceneTreeView.Title"))
			title = QStringLiteral("Scene Tree View");
		dock->setWindowTitle(title);
	}
	// Register dock via add_dock_by_id to guarantee Docks menu entry, then detach our QDockWidget shell
	bool added = false;
	QWidget *contents = dock->widget();
	if (contents) {
		// Detach contents so OBS can reparent without layout warnings
		dock->setWidget(nullptr);
		obs_frontend_add_dock_by_id("obs_scene_tree_view",
			obs_module_text("SceneTreeView.Title"), contents);
		blog(LOG_INFO, "[%s] registered via add_dock_by_id", obs_module_name());
		added = true;
	} else {
		// As a fallback, try custom_qdock path
		added = obs_frontend_add_custom_qdock("obs_scene_tree_view", dock);
		if (added)
			blog(LOG_INFO, "[%s] registered via add_custom_qdock (fallback)", obs_module_name());
	}
	g_stv_dock = dock;
	g_stv_added = added;
	obs_frontend_pop_ui_translation();

	return true;
}

MODULE_EXPORT void obs_module_unload()
{}

#define QT_UTF8(str) QString::fromUtf8(str)
#define QT_TO_UTF8(str) str.toUtf8().constData()


ObsSceneTreeView::ObsSceneTreeView(QMainWindow *main_window)
    : QDockWidget(dynamic_cast<QWidget*>(main_window)),
      _add_scene_act(main_window->findChild<QAction*>("actionAddScene")),
      _remove_scene_act(main_window->findChild<QAction*>("actionRemoveScene")),
      _toggle_toolbars_scene_act(main_window->findChild<QAction*>("toggleListboxToolbars"))
{


	config_t *const global_config = obs_frontend_get_user_config();
	config_set_default_bool(global_config, "SceneTreeView", "ShowSceneIcons", false);
	config_set_default_bool(global_config, "SceneTreeView", "ShowFolderIcons", false);

	assert(this->_add_scene_act);
	assert(this->_remove_scene_act);


		this->_move_scene_up_act = main_window->findChild<QAction*>("actionSceneUp");
		this->_move_scene_down_act = main_window->findChild<QAction*>("actionSceneDown");

	this->_stv_dock.setupUi(this);
	// Ensure dock is initially docked (not floating) after UI has set properties
	this->setAllowedAreas(Qt::AllDockWidgetAreas);
	this->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);


	this->setFloating(false);

	// Defer applying theme classes; classes are applied in FINISHED_LOADING via setClasses().
	// Disable actions until OBS finishes loading to avoid first-click race conditions

		// After model is set, track selection to update move button enabled state
		// NOTE: connect AFTER setModel; selectionModel() is null until a model is installed

	this->_stv_dock.stvAdd->setEnabled(false);
	this->_stv_dock.stvRemove->setEnabled(false);
	this->_stv_dock.stvAddFolder->setEnabled(false);
	this->_stv_dock.stvMoveUp->setEnabled(false);
	this->_stv_dock.stvMoveDown->setEnabled(false);


	this->_stv_dock.stvTree->SetItemModel(&this->_scene_tree_items);
	this->_stv_dock.stvTree->setDefaultDropAction(Qt::DropAction::MoveAction);

	// Install model into the view and then wire selection changes to keep Move Up/Down enabled state fresh
	this->_stv_dock.stvTree->setModel(&(this->_scene_tree_items));
	if (auto sm = this->_stv_dock.stvTree->selectionModel()) {
		QObject::connect(sm, &QItemSelectionModel::currentChanged, this,
				[this](const QModelIndex &, const QModelIndex &) { this->UpdateMoveButtonsEnabled(); });
		QObject::connect(sm, &QItemSelectionModel::selectionChanged, this,
				[this](const QItemSelection &, const QItemSelection &) { this->UpdateMoveButtonsEnabled(); });
	}

	const bool show_icons = config_get_bool(global_config, "BasicWindow", "ShowListboxToolbars");
	this->on_toggleListboxToolbars(show_icons);

	// Add callback to obs scene list change event
	obs_frontend_add_event_callback(&ObsSceneTreeView::obs_frontend_event_cb, this);
	obs_frontend_add_save_callback(&ObsSceneTreeView::obs_frontend_save_cb, this);

	// Resolve move up/down actions from main window for icon parity (optional)

if (this->_add_scene_act) {
	QObject::connect(this->_stv_dock.stvAdd, &QToolButton::released, this->_add_scene_act, &QAction::trigger);
} else {
	this->_stv_dock.stvAdd->setEnabled(false);
}

// Wire new move buttons

	QObject::connect(this->_stv_dock.stvTree->itemDelegate(), SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)),
	                 this, SLOT(on_SceneNameEdited(QWidget*)));
	                //main_window, SLOT(SceneNameEdited(QWidget*,QAbstractItemDelegate::EndEditHint)));

	QObject::connect(this->_toggle_toolbars_scene_act, &QAction::triggered, this, &ObsSceneTreeView::on_toggleListboxToolbars);

	// Initial compute in case a selection already exists
	this->UpdateMoveButtonsEnabled();
}

ObsSceneTreeView::~ObsSceneTreeView()
{
	// Remove frontend cb
	obs_frontend_remove_save_callback(&ObsSceneTreeView::obs_frontend_save_cb, this);
	obs_frontend_remove_event_callback(&ObsSceneTreeView::obs_frontend_event_cb, this);
}

void ObsSceneTreeView::SaveSceneTree(const char *scene_collection)
{
	if(!scene_collection)
		return;

	BPtr<char> stv_config_file_path = obs_module_config_path(SCENE_TREE_CONFIG_FILE.data());

	OBSDataAutoRelease stv_data = obs_data_create_from_json_file(stv_config_file_path);
	if(!stv_data)
		stv_data = obs_data_create();

	this->_scene_tree_items.SaveSceneTree(stv_data, scene_collection, this->_stv_dock.stvTree);

	if(!obs_data_save_json(stv_data, stv_config_file_path))
		blog(LOG_WARNING, "[%s] Failed to save scene tree in '%s'", obs_module_name(), stv_config_file_path.Get());
}

void ObsSceneTreeView::LoadSceneTree(const char *scene_collection)
{
	assert(scene_collection);

	BPtr<char> stv_config_file_path = obs_module_config_path(SCENE_TREE_CONFIG_FILE.data());

	OBSDataAutoRelease stv_data = obs_data_create_from_json_file(stv_config_file_path);
	this->_scene_tree_items.LoadSceneTree(stv_data, scene_collection, this->_stv_dock.stvTree);
}

void ObsSceneTreeView::UpdateTreeView()
{
	obs_frontend_source_list scene_list = {};
	obs_frontend_get_scenes(&scene_list);

	this->_scene_tree_items.UpdateTree(scene_list, this->_stv_dock.stvTree->currentIndex());

	obs_frontend_source_list_free(&scene_list);

	this->SaveSceneTree(this->_scene_collection_name);
}

void ObsSceneTreeView::on_toggleListboxToolbars(bool visible)
{
	this->_stv_dock.listbox->setVisible(visible);
}

void ObsSceneTreeView::on_stvAddFolder_clicked()
{
	int row;
	QStandardItem *selected = this->_scene_tree_items.itemFromIndex(this->_stv_dock.stvTree->currentIndex());
	if(!selected)
	{
		selected = this->_scene_tree_items.invisibleRootItem();
		row = selected->rowCount();
	}
	else
	{
		if(selected->type() == StvItemModel::FOLDER)
			row = selected->rowCount();
		else
		{
			row = selected->row()+1;

			selected = this->_scene_tree_items.GetParentOrRoot(selected->index());
		}


	}

	// Get unique new folder name (gracefully handle missing %1 placeholder in translation)
	QString format{obs_module_text("SceneTreeView.DefaultFolderName")};
	if (!format.contains("%1"))
		format = QStringLiteral("Folder %1");
	// Start numbering at 1 and choose the lowest available number (fills gaps)
	int i = 1;
	QString new_folder_name = format.arg(i);
	while (!this->_scene_tree_items.CheckFolderNameUniqueness(new_folder_name, selected)) {
		new_folder_name = format.arg(++i);
	}

	StvFolderItem *pItem = new StvFolderItem(new_folder_name);
	selected->insertRow(row, pItem);

	this->SaveSceneTree(this->_scene_collection_name);
}

void ObsSceneTreeView::on_stvRemove_released()
{
	QStandardItem *selected = this->_scene_tree_items.itemFromIndex(this->_stv_dock.stvTree->currentIndex());
	if(selected)
	{
		assert(selected->type() == StvItemModel::FOLDER || selected->type() == StvItemModel::SCENE);
		if(selected->type() == StvItemModel::SCENE)
			QMetaObject::invokeMethod(this->_remove_scene_act, "triggered");
		else
			this->RemoveFolder(selected);
	}
}





void ObsSceneTreeView::on_stvTree_customContextMenuRequested(const QPoint &pos)
{
	QStandardItem *item = this->_scene_tree_items.itemFromIndex(this->_stv_dock.stvTree->indexAt(pos));

	QMainWindow *main_window = reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window());

	QMenu popup(this);
//	QMenu order(QTStr("Basic.MainMenu.Edit.Order"), this);

	popup.addAction(obs_module_text("SceneTreeView.AddScene"),
	                main_window, SLOT(on_actionAddScene_triggered()));

	popup.addAction(obs_module_text("SceneTreeView.AddFolder"),
	                this, SLOT(on_stvAddFolder_clicked()));

	if(item)
	{
		if(item->type() == StvItemModel::SCENE)
		{
			QAction *copyFilters = new QAction(QTStr("Copy.Filters"), this);
			copyFilters->setEnabled(false);
			connect(copyFilters, SIGNAL(triggered()),
			        main_window, SLOT(SceneCopyFilters()));
			QAction *pasteFilters = new QAction(QTStr("Paste.Filters"), this);
//			pasteFilters->setEnabled(
//			    !obs_weak_source_expired(copyFiltersSource));			// Cannot use (we can't check copyFiltersSource, as it's a private member of OBSBasic)
			connect(pasteFilters, SIGNAL(triggered()),
			        main_window, SLOT(ScenePasteFilters()));

			popup.addSeparator();
			popup.addAction(QTStr("Duplicate"),
			                main_window, SLOT(DuplicateSelectedScene()));
			popup.addAction(copyFilters);
			popup.addAction(pasteFilters);
			popup.addSeparator();
			QAction *rename = popup.addAction(QTStr("Rename"));
			QObject::connect(rename, SIGNAL(triggered()), this->_stv_dock.stvTree, SLOT(EditSelectedItem()));
			popup.addAction(QTStr("Remove"),
			                main_window, SLOT(RemoveSelectedScene()));
			popup.addSeparator();

//			order.addAction(QTStr("Basic.MainMenu.Edit.Order.MoveUp"),
//			                main_window, SLOT(on_actionSceneUp_triggered()));
//			order.addAction(QTStr("Basic.MainMenu.Edit.Order.MoveDown"),
//			        this, SLOT(on_actionSceneDown_triggered()));
//			order.addSeparator();

//			order.addAction(QTStr("Basic.MainMenu.Edit.Order.MoveToTop"),
//			        this, SLOT(MoveSceneToTop()));
//			order.addAction(QTStr("Basic.MainMenu.Edit.Order.MoveToBottom"),
//				    this, SLOT(MoveSceneToBottom()));
//			popup.addMenu(&order);

//			popup.addSeparator();

//			delete sceneProjectorMenu;
//			sceneProjectorMenu = new QMenu(QTStr("SceneProjector"));
//			AddProjectorMenuMonitors(sceneProjectorMenu, this,
//				         SLOT(OpenSceneProjector()));
//			popup.addMenu(sceneProjectorMenu);

			QAction *sceneWindow = popup.addAction(QTStr("SceneWindow"),
			                                       main_window, SLOT(OpenSceneWindow()));

			popup.addAction(sceneWindow);
			popup.addAction(QTStr("Screenshot.Scene"),
			                main_window, SLOT(ScreenshotScene()));
			popup.addSeparator();
			popup.addAction(QTStr("Filters"),
			                main_window, SLOT(OpenSceneFilters()));

			popup.addSeparator();

			this->_per_scene_transition_menu.reset(CreatePerSceneTransitionMenu(main_window));
			popup.addMenu(this->_per_scene_transition_menu.get());

			/* ---------------------- */

			QAction *multiviewAction = popup.addAction(QTStr("ShowInMultiview"));

			OBSSourceAutoRelease source = this->_scene_tree_items.GetCurrentScene();
			OBSDataAutoRelease sceneSettings = obs_source_get_private_settings(source);

			obs_data_set_default_bool(sceneSettings, "show_in_multiview", true);
			bool show = obs_data_get_bool(sceneSettings, "show_in_multiview");

			multiviewAction->setCheckable(true);
			multiviewAction->setChecked(show);

			auto showInMultiview = [main_window](OBSData settings) {
				bool show =
				    obs_data_get_bool(settings, "show_in_multiview");
				obs_data_set_bool(settings, "show_in_multiview", !show);
				// Workaround because OBSProjector::UpdateMultiviewProjectors() isn't available to modules
				QMetaObject::invokeMethod(main_window, "ScenesReordered");
			};

			connect(multiviewAction, &QAction::triggered,
			    std::bind(showInMultiview, sceneSettings.Get()));

			copyFilters->setEnabled(obs_source_filter_count(source) > 0);
		}

		popup.addSeparator();

		// Enable/disable scene or folder icon
		const auto toggleName = item->type() == StvItemModel::SCENE ? obs_module_text("SceneTreeView.ToggleSceneIcons") :
		                                                              obs_module_text("SceneTreeView.ToggleFolderIcons");

		QAction *toggleIconAction = popup.addAction(toggleName);
		toggleIconAction->setCheckable(true);

		const auto configName = item->type() == StvItemModel::SCENE ? "ShowSceneIcons" : "ShowFolderIcons";
		const bool showIcon = config_get_bool(obs_frontend_get_user_config(), "SceneTreeView", configName);

		toggleIconAction->setChecked(showIcon);

		auto toggleIcon = [this, showIcon, configName, item]() {
			config_set_bool(obs_frontend_get_user_config(), "SceneTreeView", configName, !showIcon);
			this->_scene_tree_items.SetIconVisibility(!showIcon, (StvItemModel::QITEM_TYPE)item->type());
		};

		connect(toggleIconAction, &QAction::triggered, toggleIcon);
	}

//	popup.addSeparator();

//	bool grid = ui->scenes->GetGridMode();

//	QAction *gridAction = new QAction(grid ? QTStr("Basic.Main.ListMode")
//	                       : QTStr("Basic.Main.GridMode"),
//	                  this);
//	connect(gridAction, SIGNAL(triggered()), this,
//	    SLOT(GridActionClicked()));
//	popup.addAction(gridAction);

	popup.exec(QCursor::pos());
}

void ObsSceneTreeView::on_SceneNameEdited(QWidget *editor)
{
	QStandardItem *selected = this->_scene_tree_items.itemFromIndex(this->_stv_dock.stvTree->currentIndex());
	if(selected->type() == StvItemModel::SCENE)
	{
		QMainWindow *main_window = reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window());
		QMetaObject::invokeMethod(main_window, "SceneNameEdited", Q_ARG(QWidget*, editor));
	}
	else
	{
		QLineEdit *edit = qobject_cast<QLineEdit *>(editor);
		std::string text = QT_TO_UTF8(edit->text().trimmed());

		selected->setText(this->_scene_tree_items.CreateUniqueFolderName(selected,
		                                                                 this->_scene_tree_items.GetParentOrRoot(selected->index())));
	}
}

void ObsSceneTreeView::SelectCurrentScene()
{
	QStandardItem *item = this->_scene_tree_items.GetCurrentSceneItem();
	if(item && item->index() != this->_stv_dock.stvTree->currentIndex())
		QMetaObject::invokeMethod(this->_stv_dock.stvTree, "setCurrentIndex", Q_ARG(QModelIndex, item->index()));
}

void ObsSceneTreeView::RemoveFolder(QStandardItem *folder)
{
	int row = 0;
	int row_count = folder->rowCount();
	while(row < row_count)
	{
		QStandardItem *item = folder->child(row);
		assert(item->type() == StvItemModel::FOLDER || item->type() == StvItemModel::SCENE);

		if(item->type() == StvItemModel::SCENE)
		{
			// Keep source reference to prevent race conditions on deletion via "triggered action"
			obs_weak_source_t *weak = item->data(StvItemModel::OBS_SCENE).value<obs_weak_source_ptr>().ptr;
			OBSSourceAutoRelease source = OBSGetStrongRef(weak);

			this->_scene_tree_items.SetSelectedScene(item, obs_frontend_preview_program_mode_active());
			QMetaObject::invokeMethod(this->_remove_scene_act, "triggered");
		}
		else
			this->RemoveFolder(item);

		// Check if item was deleted. If not, move to next row
		if(row_count == folder->rowCount())
			++row;

		row_count = folder->rowCount();
	}

	// Remove folder if empty
	if(folder->rowCount() == 0)
		this->_scene_tree_items.GetParentOrRoot(folder->index())->removeRow(folder->row());
}

Q_DECLARE_METATYPE(OBSSource);

static inline OBSSource GetTransitionComboItem(QComboBox *combo, int idx)
{
	return combo->itemData(idx).value<OBSSource>();
}

QMenu *ObsSceneTreeView::CreatePerSceneTransitionMenu(QMainWindow *main_window)
{
	OBSSourceAutoRelease scene = this->_scene_tree_items.GetCurrentScene();
	QMenu *menu = new QMenu(QTStr("TransitionOverride"));
	QAction *action;

	OBSDataAutoRelease sceneSettings = obs_source_get_private_settings(scene);

	obs_data_set_default_int(sceneSettings, "transition_duration", 300);

	const char *curTransition = obs_data_get_string(sceneSettings, "transition");
	int curDuration = (int)obs_data_get_int(sceneSettings, "transition_duration");

	QSpinBox *duration = new QSpinBox(menu);
	duration->setMinimum(50);
	duration->setSuffix(" ms");
	duration->setMaximum(20000);
	duration->setSingleStep(50);
	duration->setValue(curDuration);

	// Workaround to get the transitions menu from the main menu
	QComboBox *combo = main_window->findChild<QComboBox*>("transitions");
	assert(combo);

	auto setTransition = [this, combo](QAction *action) {
		int idx = action->property("transition_index").toInt();
		OBSSourceAutoRelease scene = this->_scene_tree_items.GetCurrentScene();
		OBSDataAutoRelease sceneSettings =
		    obs_source_get_private_settings(scene);

		if (idx == -1) {
			obs_data_set_string(sceneSettings, "transition", "");
			return;
		}

		OBSSource tr = GetTransitionComboItem(combo, idx);

		if (tr)
		{
			const char *name = obs_source_get_name(tr);
			obs_data_set_string(sceneSettings, "transition", name);
		}
	};

	auto setDuration = [this](int duration) {
		OBSSourceAutoRelease scene = this->_scene_tree_items.GetCurrentScene();
		OBSDataAutoRelease sceneSettings =
		    obs_source_get_private_settings(scene);

		obs_data_set_int(sceneSettings, "transition_duration", duration);
	};

	connect(duration, (void (QSpinBox::*)(int)) & QSpinBox::valueChanged, setDuration);

	std::string none = "None";
	for (int i = -1; i < combo->count(); i++)
	{
		const char *name = "";

		if (i >= 0)
		{
			OBSSource tr;
			tr = GetTransitionComboItem(combo, i);
			if (!tr)
				continue;
			name = obs_source_get_name(tr);
		}

		bool match = (name && strcmp(name, curTransition) == 0);

		if (!name || !*name)
			name = none.c_str();

		action = menu->addAction(QT_UTF8(name));
		action->setProperty("transition_index", i);
		action->setCheckable(true);
		action->setChecked(match);

		connect(action, &QAction::triggered,
		    std::bind(setTransition, action));
	}

	QWidgetAction *durationAction = new QWidgetAction(menu);
	durationAction->setDefaultWidget(duration);

	menu->addSeparator();
	menu->addAction(durationAction);
	return menu;
}

void ObsSceneTreeView::ObsFrontendEvent(enum obs_frontend_event event)
{
	// Update our tree view when scene list was changed

	if(event == OBS_FRONTEND_EVENT_FINISHED_LOADING)
	{
		// Retry dock registration if it failed during module load (e.g., early lifecycle)
		if (!g_stv_added && g_stv_dock) {
			bool added = false;
			// Prefer add_dock_by_id to ensure Docks menu entry
			QWidget *contents = g_stv_dock->widget();
			if (contents) {
				g_stv_dock->setWidget(nullptr);
				obs_frontend_add_dock_by_id("obs_scene_tree_view",
					obs_module_text("SceneTreeView.Title"), contents);
				blog(LOG_INFO, "[%s] retry add_dock_by_id invoked", obs_module_name());
				added = true;
			} else {
				// Fallback: try custom_qdock
				added = obs_frontend_add_custom_qdock("obs_scene_tree_view", g_stv_dock);
				if (added)
					blog(LOG_INFO, "[%s] add_custom_qdock retry succeeded", obs_module_name());
			}
			g_stv_added = added;
		}

		this->_scene_collection_name = obs_frontend_get_current_scene_collection();

		// Load saved scene locations, then add any missing items that weren't saved
		this->LoadSceneTree(this->_scene_collection_name);
		this->UpdateTreeView();

		this->SelectCurrentScene();

		// Apply icons and theme classes; reusable for theme changes and initial load
		auto applyThemeAndIcons = [this]() {
			QMainWindow *main_window = reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window());
			// Base icons (theme may override via qproperty-icon on class selectors)
			if (this->_add_scene_act)
				this->_stv_dock.stvAdd->setIcon(this->_add_scene_act->icon());
			if (this->_remove_scene_act)
				this->_stv_dock.stvRemove->setIcon(this->_remove_scene_act->icon());
			this->_stv_dock.stvAddFolder->setIcon(main_window->property("groupIcon").value<QIcon>());

				// Up/Down icons (non-dimmed when disabled)
				if (this->_move_scene_up_act)
					this->_stv_dock.stvMoveUp->setIcon(NonDimmedDisabled(this->_move_scene_up_act->icon()));
				if (this->_move_scene_down_act)
					this->_stv_dock.stvMoveDown->setIcon(NonDimmedDisabled(this->_move_scene_down_act->icon()));


			// Copy QAction dynamic properties to buttons (mirrors OBS behavior)
			auto copyProps = [](QAction *act, QWidget *w) {

				if (!act || !w) return;
				const auto names = act->dynamicPropertyNames();
				for (const QByteArray &n : names)
					w->setProperty(n.constData(), act->property(n.constData()));
			};
			copyProps(this->_add_scene_act, this->_stv_dock.stvAdd);
			copyProps(this->_remove_scene_act, this->_stv_dock.stvRemove);

			// Compose class strings to enable theme icon overrides + our button styling
			QString addCls = this->_add_scene_act ? this->_add_scene_act->property("class").toString() : QString();

				copyProps(this->_move_scene_up_act, this->_stv_dock.stvMoveUp);
				copyProps(this->_move_scene_down_act, this->_stv_dock.stvMoveDown);

			QString remCls = this->_remove_scene_act ? this->_remove_scene_act->property("class").toString() : QString();

			QString addClasses = QStringLiteral("btn-tool");
			if (!addCls.isEmpty()) addClasses += QStringLiteral(" ") + addCls; else addClasses += QStringLiteral(" icon-plus");
			QString removeClasses = QStringLiteral("btn-tool");
			if (!remCls.isEmpty()) removeClasses += QStringLiteral(" ") + remCls; else removeClasses += QStringLiteral(" icon-trash");


				QString upCls = this->_move_scene_up_act ? this->_move_scene_up_act->property("class").toString() : QString();
				QString downCls = this->_move_scene_down_act ? this->_move_scene_down_act->property("class").toString() : QString();

				QString upClasses = QStringLiteral("btn-tool");
				if (!upCls.isEmpty()) upClasses += QStringLiteral(" ") + upCls; else upClasses += QStringLiteral(" icon-up");
				QString downClasses = QStringLiteral("btn-tool");
				if (!downCls.isEmpty()) downClasses += QStringLiteral(" ") + downCls; else downClasses += QStringLiteral(" icon-down");

			setClasses(this->_stv_dock.stvAdd, addClasses);
			setClasses(this->_stv_dock.stvRemove, removeClasses);
			setClasses(this->_stv_dock.stvAddFolder, QStringLiteral("btn-tool"));

				setClasses(this->_stv_dock.stvMoveUp, upClasses);
				setClasses(this->_stv_dock.stvMoveDown, downClasses);


			// Force stylesheet recalculation (belt-and-suspenders)
			QString qss = this->styleSheet();
			this->setStyleSheet("/* */");
			this->setStyleSheet(qss);
		};

		applyThemeAndIcons();

		// Re-enable toolbar buttons now that OBS has finished loading
		this->_stv_dock.stvAdd->setEnabled(true);
		this->_stv_dock.stvRemove->setEnabled(true);
		this->_stv_dock.stvAddFolder->setEnabled(true);

			this->_stv_dock.stvMoveUp->setEnabled(true);
			this->_stv_dock.stvMoveDown->setEnabled(true);
			this->UpdateMoveButtonsEnabled();


	}
		else if(event == OBS_FRONTEND_EVENT_THEME_CHANGED)
		{
			// Reapply icons and theme classes when user switches themes at runtime
			QMainWindow *main_window = reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window());
			if (this->_add_scene_act)
				this->_stv_dock.stvAdd->setIcon(this->_add_scene_act->icon());
			if (this->_remove_scene_act)
				this->_stv_dock.stvRemove->setIcon(this->_remove_scene_act->icon());
			this->_stv_dock.stvAddFolder->setIcon(main_window->property("groupIcon").value<QIcon>());

				// Up/Down icons (non-dimmed when disabled)
				if (this->_move_scene_up_act)
					this->_stv_dock.stvMoveUp->setIcon(NonDimmedDisabled(this->_move_scene_up_act->icon()));
				if (this->_move_scene_down_act)
					this->_stv_dock.stvMoveDown->setIcon(NonDimmedDisabled(this->_move_scene_down_act->icon()));


			auto copyProps = [](QAction *act, QWidget *w) {
				if (!act || !w) return;

				const auto names = act->dynamicPropertyNames();
				for (const QByteArray &n : names)
					w->setProperty(n.constData(), act->property(n.constData()));
			};
			copyProps(this->_add_scene_act, this->_stv_dock.stvAdd);
			copyProps(this->_remove_scene_act, this->_stv_dock.stvRemove);


				copyProps(this->_move_scene_up_act, this->_stv_dock.stvMoveUp);
				copyProps(this->_move_scene_down_act, this->_stv_dock.stvMoveDown);

			QString addCls = this->_add_scene_act ? this->_add_scene_act->property("class").toString() : QString();
			QString remCls = this->_remove_scene_act ? this->_remove_scene_act->property("class").toString() : QString();

			QString addClasses = QStringLiteral("btn-tool");
			if (!addCls.isEmpty()) addClasses += QStringLiteral(" ") + addCls; else addClasses += QStringLiteral(" icon-plus");
			QString removeClasses = QStringLiteral("btn-tool");

				QString upCls = this->_move_scene_up_act ? this->_move_scene_up_act->property("class").toString() : QString();
				QString downCls = this->_move_scene_down_act ? this->_move_scene_down_act->property("class").toString() : QString();

				if (!remCls.isEmpty()) removeClasses += QStringLiteral(" ") + remCls; else removeClasses += QStringLiteral(" icon-trash");
				QString upClasses = QStringLiteral("btn-tool");
				if (!upCls.isEmpty()) upClasses += QStringLiteral(" ") + upCls; else upClasses += QStringLiteral(" icon-up");
				QString downClasses = QStringLiteral("btn-tool");
				if (!downCls.isEmpty()) downClasses += QStringLiteral(" ") + downCls; else downClasses += QStringLiteral(" icon-down");

				setClasses(this->_stv_dock.stvAdd, addClasses);
				setClasses(this->_stv_dock.stvRemove, removeClasses);
				setClasses(this->_stv_dock.stvAddFolder, QStringLiteral("btn-tool"));
				setClasses(this->_stv_dock.stvMoveUp, upClasses);
				setClasses(this->_stv_dock.stvMoveDown, downClasses);

			QString qss = this->styleSheet();
			this->setStyleSheet("/* */");
			this->setStyleSheet(qss);
		}

	else if(event == OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED)
		this->UpdateTreeView();
	else if(event == OBS_FRONTEND_EVENT_SCENE_CHANGED || event == OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED)
		this->SelectCurrentScene();
	else if(event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP)
	{
		this->_scene_tree_items.CleanupSceneTree();
		this->_scene_collection_name = nullptr;
	}
	else if(event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING)
		this->SaveSceneTree(this->_scene_collection_name);
	else if(event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED)
	{
		this->_scene_collection_name = obs_frontend_get_current_scene_collection();
		this->LoadSceneTree(this->_scene_collection_name);
		this->UpdateTreeView();
	}
	else if(event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_RENAMED)
	{
		// TODO: Delete old scene tree from json file

		this->_scene_collection_name = obs_frontend_get_current_scene_collection();
		this->SaveSceneTree(this->_scene_collection_name);

		this->UpdateTreeView();
	}
}

void ObsSceneTreeView::ObsFrontendSave(obs_data_t */*save_data*/, bool saving)
{
	if(saving)
		this->SaveSceneTree(this->_scene_collection_name);
}


void ObsSceneTreeView::on_stvMoveUp_released()
{
	const QModelIndex idx = this->_stv_dock.stvTree->currentIndex();
	if (!idx.isValid())
		return;
	const int oldRow = idx.row();
	QStandardItem *parent = this->_scene_tree_items.GetParentOrRoot(idx);
	if (this->_scene_tree_items.MoveIndexByOne(idx, -1)) {
		this->SaveSceneTree(this->_scene_collection_name);
		if (parent && oldRow - 1 >= 0 && oldRow - 1 < parent->rowCount())
			this->_stv_dock.stvTree->setCurrentIndex(parent->child(oldRow - 1)->index());
	}
	this->UpdateMoveButtonsEnabled();
}

void ObsSceneTreeView::on_stvMoveDown_released()
{
	const QModelIndex idx = this->_stv_dock.stvTree->currentIndex();
	if (!idx.isValid())
		return;
	const int oldRow = idx.row();
	QStandardItem *parent = this->_scene_tree_items.GetParentOrRoot(idx);
	if (this->_scene_tree_items.MoveIndexByOne(idx, +1)) {
		this->SaveSceneTree(this->_scene_collection_name);
		// With corrected move-down logic, moved item ends at oldRow + 1
		if (parent && oldRow + 1 < parent->rowCount())
			this->_stv_dock.stvTree->setCurrentIndex(parent->child(oldRow + 1)->index());
	}
	this->UpdateMoveButtonsEnabled();
}

void ObsSceneTreeView::UpdateMoveButtonsEnabled()
{
	bool enableUp = false;
	bool enableDown = false;
	const QModelIndex idx = this->_stv_dock.stvTree->currentIndex();
	if (idx.isValid()) {
		QStandardItem *parent = this->_scene_tree_items.GetParentOrRoot(idx);
		const int row = idx.row();
		const int count = parent ? parent->rowCount() : 0;
		enableUp = (row > 0);
		enableDown = (row >= 0 && row < count - 1);
	}
	this->_stv_dock.stvMoveUp->setEnabled(enableUp);
	this->_stv_dock.stvMoveDown->setEnabled(enableDown);
}
