#include "stdafx.h"
#include "ListView.h"
#include "GtkSignal.h"

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

		*rows << Row(row);
		modelAdd(row);
		return rows->count() - 1;
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

#ifdef GUI_GTK

	bool ListView::create(ContainerBase *parent, nat id) {
		gint columns = gint(cols->count());
		GType *types = new GType[columns];
		for (gint i = 0; i < columns; i++)
			types[i] = G_TYPE_STRING;
		gtkStore = gtk_list_store_newv(columns, types);
		delete[] types;

		GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(gtkStore));

		for (Nat i = 0; i < cols->count(); i++) {
			GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

			GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes(
				cols->at(i)->utf8_str(),
				renderer,
				"text", gint(i),
				NULL);
			gtk_tree_view_column_set_resizable(col, true);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
		}

		// Add initial data.
		for (Nat i = 0; i < rows->count(); i++) {
			modelAdd(rows->at(i).cols);
		}

		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), showHeader);
		gtk_widget_show(tree);

		GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(scrolled), tree);

		initWidget(parent, scrolled);
		// signals...
		return true;
	}

	void ListView::modelAdd(Array<Str *> *row) {
		if (!gtkStore)
			return;

		GtkTreeIter iter;
		gtk_list_store_append(gtkStore, &iter);
		for (Nat c = 0; c < row->count(); c++) {
			gtk_list_store_set(gtkStore, &iter, c, row->at(c)->utf8_str(), -1);
		}
	}

	void ListView::modelRemove(Nat id) {
		if (!gtkStore)
			return;

		GtkTreeModel *model = GTK_TREE_MODEL(gtkStore);

		GtkTreeIter iter;
		gtk_tree_model_get_iter_first(model, &iter);
		for (Nat i = 0; i < id; i++)
			gtk_tree_model_iter_next(model, &iter);
		gtk_list_store_remove(gtkStore, &iter);
	}

	void ListView::modelClear() {
		if (!gtkStore)
			return;

		gtk_list_store_clear(gtkStore);
	}

	void ListView::clearData() {
		if (gtkStore) {
			g_object_unref(G_OBJECT(gtkStore));
			gtkStore = null;
		}
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

#endif

}
