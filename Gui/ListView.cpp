#include "stdafx.h"
#include "ListView.h"
#include "GtkSignal.h"
#include "Container.h"
#include "Win32Dpi.h"

// #ifdef WIN32
// #include <uxtheme.h>
// #pragma comment (lib, "uxtheme.lib")
// #endif

namespace gui {

	ListView::ListView() {
		this->showHeader = false;
		this->cols = new (this) Array<Str *>();
		this->rows = new (this) Array<Row>();
		this->cols->push(new (this) Str(S("")));
	}

	ListView::ListView(Array<Str *> *cols) {
		if (cols->empty())
			throw new (this) ListViewError(S("At least one column should be specified to the list view."));

		showHeader = true;

		this->cols = new (this) Array<Str *>(*cols); // copy to avoid accidental modifications
		this->rows = new (this) Array<Row>();
	}

	ListView::~ListView() {
		clearData();
	}

	Nat ListView::add(Array<Str *> *row) {
		if (row->count() != cols->count()) {
			throw new (this) ListViewError(
				TO_S(this, S("Trying to add ") << row->count()
					<< S(" columns to a ListView containing ")
					<< cols->count() << S(" columns.")));
		}

		Nat id = rows->count();
		*rows << Row(row);
		modelAdd(row, id);
		return id;
	}

	Nat ListView::count() const {
		return rows->count();
	}

	void ListView::remove(Nat id) {
		rows->remove(id);
		modelRemove(id);
	}

	void ListView::clear() {
		rows->clear();
		modelClear();
	}

	void ListView::destroyWindow(Handle handle) {
		Window::destroyWindow(handle);
		clearData();
	}

#ifdef GUI_WIN32

	static void addRow(HWND hwnd, Array<Str *> *row, Nat index) {
		LVITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = index;
		item.iSubItem = 0;
		item.pszText = (LPWSTR)row->at(0)->c_str();

		// In case 'index' is too large (happens when we insert initially).
		item.iItem = ListView_InsertItem(hwnd, &item);

		for (Nat i = 1; i < row->count(); i++) {
			item.iSubItem = i;
			item.pszText = (LPWSTR)row->at(i)->c_str();
			ListView_SetItem(hwnd, &item);
		}
	}

	bool ListView::create(ContainerBase *parent, nat id) {
		DWORD flags = controlFlags | LVS_REPORT;
		if (!showHeader)
			flags |= LVS_NOCOLUMNHEADER;
		if (!multiSel)
			flags |= LVS_SINGLESEL;
		if (!Window::createEx(WC_LISTVIEW, flags, WS_EX_CLIENTEDGE, parent->handle().hwnd(), id))
			return false;

		HWND hwnd = handle().hwnd();

		// Note: Looks nicer, but has visual artifacts when resizing columns in a list that is not entirely full.
		// SetWindowTheme(hwnd, S("Explorer"), NULL);

		ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT);

		// Note: If we want to have a non-left aligned column first, we need to add a dummy element
		// as the first element and then remove it (that is what MSDN says).

		for (Nat i = 0; i < cols->count(); i++) {
			LVCOLUMN column;
			column.mask = LVCF_FMT | LVCF_TEXT;
			column.fmt = LVCFMT_LEFT;
			column.pszText = (LPWSTR)cols->at(i)->c_str();

			ListView_InsertColumn(hwnd, i, &column);
		}

		// Add a temporary last column, then autosize the columns based on the headers. The last
		// column makes it so that it won't fill the entire width of the control.
		{
			LVCOLUMN column;
			column.mask = LVCF_FMT | LVCF_TEXT;
			column.fmt = LVCFMT_LEFT;
			column.pszText = (LPWSTR)S("");
			ListView_InsertColumn(hwnd, cols->count(), &column);
		}

		for (Nat i = 0; i < cols->count(); i++) {
			ListView_SetColumnWidth(hwnd, i, LVSCW_AUTOSIZE_USEHEADER);

			LVCOLUMN column;
			column.mask = LVCF_MINWIDTH;
			column.cxMin = ListView_GetColumnWidth(hwnd, i);
			ListView_SetColumn(hwnd, i, &column);
		}

		ListView_DeleteColumn(hwnd, cols->count());

		// Add data.
		for (Nat i = 0; i < rows->count(); i++)
			addRow(hwnd, rows->at(i).cols, i);

		// Finally, do some resizing to account for data.
		for (Nat i = 0; i < cols->count(); i++)
			ListView_SetColumnWidth(hwnd, i, LVSCW_AUTOSIZE);

		if (selected) {
			selection(selected);
			selected = null;
		}

		return true;
	}

	bool ListView::onNotify(NMHDR *header) {
		switch (header->code) {
		case LVN_ITEMCHANGED: {
			NMLISTVIEW *view = (NMLISTVIEW *)header;
			bool oldState = (view->uOldState & LVIS_SELECTED) != 0;
			bool newState = (view->uNewState & LVIS_SELECTED) != 0;

			if (oldState != newState && onSelect) {
				onSelect->call(Nat(view->iItem), newState);
			}
			break;
		}
		case LVN_ITEMACTIVATE: {
			NMITEMACTIVATE *activate = (NMITEMACTIVATE *)header;
			if (onActivate)
				onActivate->call(activate->iItem);
			break;
		}
		}

		return true;
	}

	void ListView::modelAdd(Array<Str *> *row, Nat index) {
		if (!created())
			return;

		HWND hwnd = handle().hwnd();
		addRow(hwnd, row, index);

		// Update size.
		Font *font = this->font();
		Nat dpi = currentDpi();
		Nat padding = dpiSystemMetrics(SM_CXEDGE, dpi) * 6; // from StackOverflow.
		for (Nat i = 0; i < cols->count(); i++) {
			Size sz = font->stringSize(row->at(i), dpi);
			sz.w += padding;
			int width = ListView_GetColumnWidth(hwnd, i);
			// int hspace = ListView_GetItemSpacing(hwnd,
			if (sz.w > width)
				ListView_SetColumnWidth(hwnd, i, int(sz.w));
		}
	}

	void ListView::modelRemove(Nat id) {
		if (!created())
			return;

		ListView_DeleteItem(handle().hwnd(), id);
	}

	void ListView::modelClear() {
		if (!created())
			return;

		ListView_DeleteAllItems(handle().hwnd());
	}

	void ListView::clearData() {
		// nothing needed here
	}

	void ListView::multiSelect(Bool v) {
		multiSel = v;
		if (created()) {
			LONG_PTR old = GetWindowLong(handle().hwnd(), GWL_STYLE);
			if (multiSel)
				old &= ~LVS_SINGLESEL;
			else
				old |= LVS_SINGLESEL;
			SetWindowLong(handle().hwnd(), GWL_STYLE, old);
		}
	}

	void ListView::selection(Set<Nat> *sel) {
		if (!created()) {
			selected = sel;
			return;
		}

		HWND hwnd = handle().hwnd();
		for (Nat i = 0; i < rows->count(); i++) {
			LVITEM item;
			item.iItem = i;
			item.iSubItem = 0;
			item.mask = LVIF_STATE;
			item.state = sel->has(i) ? LVIS_SELECTED : 0;
			item.stateMask = LVIS_SELECTED;
			ListView_SetItem(hwnd, &item);
		}
	}

	Set<Nat> *ListView::selection() {
		if (!created()) {
			if (!selected)
				selected = new (this) Set<Nat>();
			return selected;
		}

		Set<Nat> *result = new (this) Set<Nat>();
		HWND hwnd = handle().hwnd();
		for (Nat i = 0; i < rows->count(); i++) {
			LVITEM item;
			item.iItem = i;
			item.iSubItem = 0;
			item.mask = LVIF_STATE;
			item.stateMask = LVIS_SELECTED;
			ListView_GetItem(hwnd, &item);

			if (item.state & LVIS_SELECTED)
				result->put(i);
		}
		return result;
	}

	Size ListView::minSize() {
		if (!created())
			return Size(10, 10);

		HWND hwnd = handle().hwnd();

		Nat width = 0;
		for (Nat i = 0; i < cols->count(); i++) {
			LVCOLUMN column;
			column.mask = LVCF_MINWIDTH;
			ListView_GetColumn(hwnd, i, &column);
			width += column.cxMin;
		}

		// Assume we will need a vertical scroll bar.
		width += dpiSystemMetrics(SM_CXVSCROLL, currentDpi());

		return Size(Float(width), 30);
	}

#endif
#ifdef GUI_GTK

	static void updateSelection(GtkTreeView *tree, GtkTreeModel *model, Set<Nat> *selection);

	struct SelectData {
		Engine *engine;
		GtkWidget *widget;
	};

	static void freeSelectData(gpointer pointer) {
		delete (SelectData *)pointer;
	}

	bool ListView::create(ContainerBase *parent, nat id) {
		gint columns = gint(cols->count());
		// We store the row ID as the first column. This lets us support sort orders easily in the
		// future and makes selection easier to handle.
		GType *types = new GType[columns + 1];
		types[0] = G_TYPE_UINT;
		for (gint i = 1; i < columns + 1; i++)
			types[i] = G_TYPE_STRING;
		gtkStore = gtk_list_store_newv(columns + 1, types);
		delete[] types;

		GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(gtkStore));

		for (Nat i = 0; i < cols->count(); i++) {
			GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

			GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes(
				cols->at(i)->utf8_str(),
				renderer,
				"text", gint(i + 1),
				NULL);
			gtk_tree_view_column_set_resizable(col, true);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
		}

		// Add initial data.
		for (Nat i = 0; i < rows->count(); i++) {
			modelAdd(rows->at(i).cols, i);
		}

		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), showHeader);
		gtk_widget_show(tree);

		GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(scrolled), tree);

		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
		gtk_tree_selection_set_mode(selection, multiSel ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE);

		if (selected) {
			updateSelection(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(gtkStore), selected);
			selected = null;
		}

		initWidget(parent, scrolled);

		Signal<void, ListView, GtkTreePath *, GtkTreeViewColumn *>::Connect<&ListView::rowActivated>::to(tree, "row-activated", engine());
		SelectData *data = new SelectData;
		data->engine = &engine();
		data->widget = tree;
		gtk_tree_selection_set_select_function(selection, &ListView::selectFunction, data, &freeSelectData);

		return true;
	}

	gboolean ListView::selectFunction(GtkTreeSelection *selection,
									GtkTreeModel *model,
									GtkTreePath *path,
									gboolean currently_selected,
									gpointer data) {

		SelectData *d = (SelectData *)data;
		App *app = gui::app(*d->engine);

		GtkWidget *at = d->widget;
		Window *win = null;
		while ((win = app->findWindow(Handle(at))) == null) {
			at = gtk_widget_get_parent(at);
			if (!at) {
				WARNING(L"Unknown window: " << d->widget);
				return TRUE;
			}
		}

		ListView *me = as<ListView>(win);
		if (me)
			me->rowSelected(selection, model, path, currently_selected);

		return TRUE;
	}

	void ListView::rowSelected(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean current) {
		if (onSelect) {
			GtkTreeIter iter;
			gtk_tree_model_get_iter(model, &iter, path);

			GValue value = {};
			gtk_tree_model_get_value(model, &iter, 0, &value);
			Nat row = g_value_get_uint(&value);
			g_value_unset(&value);

			// About to toggle to selected?
			if (current == 0) {
				if (row == lastSelected) {
					row = -1;
					return;
				}
				lastSelected = row;
			}

			onSelect->call(row, current == 0);
		}
	}

	void ListView::rowActivated(GtkTreePath *path, GtkTreeViewColumn *column) {
		if (onActivate) {
			GtkTreeModel *model = GTK_TREE_MODEL(gtkStore);

			GtkTreeIter iter;
			gtk_tree_model_get_iter(model, &iter, path);

			GValue value = {};
			gtk_tree_model_get_value(model, &iter, 0, &value);
			Nat row = g_value_get_uint(&value);
			g_value_unset(&value);

			onActivate->call(row);
		}
	}

	void ListView::modelAdd(Array<Str *> *row, Nat id) {
		if (!gtkStore)
			return;

		lastSelected = -1;

		GtkTreeIter iter;
		gtk_list_store_append(gtkStore, &iter);
		gtk_list_store_set(gtkStore, &iter, 0, guint(id), -1);
		for (Nat c = 0; c < row->count(); c++) {
			gtk_list_store_set(gtkStore, &iter, c + 1, row->at(c)->utf8_str(), -1);
		}
	}

	void ListView::modelRemove(Nat id) {
		if (!gtkStore)
			return;

		lastSelected = -1;

		GtkTreeModel *model = GTK_TREE_MODEL(gtkStore);

		GtkTreeIter iter;
		gtk_tree_model_get_iter_first(model, &iter);

		GValue value = {};
		do {
			gtk_tree_model_get_value(model, &iter, 0, &value);
			Nat rowId = g_value_get_uint(&value);
			g_value_unset(&value);

			if (rowId == id) {
				gtk_list_store_remove(gtkStore, &iter);
				break;
			}
		} while (gtk_tree_model_iter_next(model, &iter));
	}

	void ListView::modelClear() {
		if (!gtkStore)
			return;

		lastSelected = -1;

		gtk_list_store_clear(gtkStore);
	}

	void ListView::clearData() {
		if (gtkStore) {
			g_object_unref(G_OBJECT(gtkStore));
			gtkStore = null;
		}
	}

	void ListView::multiSelect(Bool v) {
		multiSel = v;

		if (created()) {
			GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(fontWidget()));
			gtk_tree_selection_set_mode(selection, multiSel ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE);
		}
	}

	static void captureSelected(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
		Set<Nat> *result = (Set<Nat> *)data;

		GValue value = {};
		gtk_tree_model_get_value(model, iter, 0, &value);
		result->put(g_value_get_uint(&value));
		g_value_unset(&value);
	}

	Set<Nat> *ListView::selection() {
		if (!created()) {
			if (!selected)
				selected = new (this) Set<Nat>();
			return selected;
		}

		Set<Nat> *result = new (this) Set<Nat>();
		GtkTreeSelection *selection = gtk_tree_view_get_selection(viewWidget());

		// Note: It is OK to pass 'result' to this function even though it is GCd. It resides on the stack.
		gtk_tree_selection_selected_foreach(selection, &captureSelected, result);

		return result;
	}

	static void updateSelection(GtkTreeView *tree, GtkTreeModel *model, Set<Nat> *selection) {
		GValue value = {};
		GtkTreeIter iter;
		GtkTreeSelection *treeSel = gtk_tree_view_get_selection(tree);
		gtk_tree_model_get_iter_first(model, &iter);

		do {
			gtk_tree_model_get_value(model, &iter, 0, &value);
			Nat id = g_value_get_uint(&value);
			g_value_unset(&value);

			if (selection->has(id))
				gtk_tree_selection_select_iter(treeSel, &iter);
			else
				gtk_tree_selection_unselect_iter(treeSel, &iter);

		} while (gtk_tree_model_iter_next(model, &iter));
	}

	void ListView::selection(Set<Nat> *sel) {
		if (!created()) {
			selected = sel;
			return;
		}

		updateSelection(viewWidget(), GTK_TREE_MODEL(gtkStore), sel);
	}

	Size ListView::minSize() {
		gint w = 0, h = 0;

		if (created()) {
			gtk_widget_get_preferred_width(handle().widget(), &w, NULL);
			gtk_widget_get_preferred_height(handle().widget(), &h, NULL);
		}

		return Size(Float(w), Float(h));
	}

	GtkWidget *ListView::fontWidget() {
		return gtk_bin_get_child(GTK_BIN(handle().widget()));
	}

	GtkTreeView *ListView::viewWidget() const {
		return GTK_TREE_VIEW(gtk_bin_get_child(GTK_BIN(handle().widget())));
	}

#endif

}
