#pragma once
#include "Window.h"
#include "Exception.h"

namespace gui {

	/**
	 * List-view.
	 *
	 * Displays a list of data. Elements may contain columns.
	 */
	class ListView : public Window {
		STORM_CLASS;
	public:
		// Create a list view without columns.
		STORM_CTOR ListView();

		// Create the list view, specify columns to show.
		STORM_CTOR ListView(Array<Str *> *columns);

		// Destructor.
		~ListView();

		// Add a row. Returns ID of newly created row.
		Nat STORM_FN add(Array<Str *> *row);

		// Remove a row.
		void STORM_FN remove(Nat id);

		// Clear entire data contents.
		void STORM_FN clear();

		// Count number of elements.
		Nat STORM_FN count() const;

#ifdef GUI_GTK
		virtual GtkWidget *fontWidget();
#endif

		// Get minimum size.
		virtual Size STORM_FN minSize();

	protected:
		virtual bool create(ContainerBase *parent, nat id);
		virtual void destroyWindow(Handle handle);

	private:

		// A single row in the list view.
		class Row {
			STORM_VALUE;
		public:
			Row(Array<Str *> *cols)
				: cols(cols) {}

			// Columns.
			Array<Str *> *cols;
		};

		// Column headers.
		Array<Str *> *cols;

		// Data to show.
		Array<Row> *rows;

		// For GTK: TreeStore object.
		UNKNOWN(PTR_NOGC) GtkListStore *gtkStore;

		// Header visible?
		Bool showHeader;

		// Add to model.
		void modelAdd(Array<Str *> *row);

		// Remove from model.
		void modelRemove(Nat id);

		// Clear model.
		void modelClear();

		// Clear any local allocations.
		void clearData();
	};


	/**
	 * Custom errors for the list.
	 */
	class EXCEPTION_EXPORT ListViewError : public GuiError {
		STORM_EXCEPTION;
	public:
		ListViewError(const wchar *what) : GuiError(what) {}
		STORM_CTOR ListViewError(Str *what) : GuiError(what) {}
	};

}
