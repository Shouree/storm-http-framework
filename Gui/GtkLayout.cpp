#include "stdafx.h"
#include "GtkLayout.h"

#ifdef GUI_GTK


G_DEFINE_TYPE(Basic, basic, GTK_TYPE_CONTAINER)

enum {
	CHILD_PROP_0,
	CHILD_PROP_X,
	CHILD_PROP_Y,
	CHILD_PROP_W,
	CHILD_PROP_H,
};

static GType basic_child_type(GtkContainer *container) {
	return GTK_TYPE_WIDGET;
}

static BasicChild *get_child(Basic *me, GtkWidget *widget) {
	for (GList *at = me->children; at; at = at->next) {
		BasicChild *child = (BasicChild *)at->data;
		if (child->widget == widget)
			return child;
	}
	return null;
}

static void move(Basic *me, BasicChild *child, gint x, gint y, gint w, gint h) {
	gtk_widget_freeze_child_notify(child->widget);

	if (child->x != x)
		gtk_widget_child_notify(child->widget, "x");
	if (child->y != y)
		gtk_widget_child_notify(child->widget, "y");
	if (child->w != w)
		gtk_widget_child_notify(child->widget, "w");
	if (child->h != h)
		gtk_widget_child_notify(child->widget, "h");

	child->x = x;
	child->y = y;
	child->w = w;
	child->h = h;

	gtk_widget_thaw_child_notify(child->widget);
	// This breaks scrollbars in GtkScrolledWindow...
	// if (gtk_widget_get_visible(child->widget) && gtk_widget_get_visible(GTK_WIDGET(me)))
	// 	gtk_widget_queue_resize(GTK_WIDGET(me));
}

void basic_put(Basic *layout, GtkWidget *widget, gint x, gint y, gint w, gint h) {
	if (w < 0)
		w = 0;
	if (h < 0)
		h = 0;

	BasicChild *child = g_new(BasicChild, 1);
	child->widget = widget;
	child->x = x;
	child->y = y;
	child->w = w;
	child->h = h;

	gtk_widget_set_parent(widget, GTK_WIDGET(layout));
	layout->children = g_list_append(layout->children, child);
}

void basic_move(Basic *layout, GtkWidget *widget, gint x, gint y, gint w, gint h) {
	if (w < 0)
		w = 0;
	if (h < 0)
		h = 0;

	// Child coordinates seem to be relative to the same origin as the allocation of this widget.
	GtkAllocation allocation;
	gtk_widget_get_allocation(GTK_WIDGET(layout), &allocation);

	BasicChild *child = get_child(layout, widget);
	assert(child, L"No child info!");
	move(layout, child, x, y, w, h);

	// Update the allocation of the widget as well.
	GtkAllocation alloc;
	alloc.x = allocation.x + x;
	alloc.y = allocation.y + y;
	alloc.width = w;
	alloc.height = h;
	gtk_widget_size_allocate(widget, &alloc);
}

static void basic_set_property(GtkContainer *here, GtkWidget *child, guint id, const GValue *value, GParamSpec *spec) {
	Basic *me = BASIC(here);
	BasicChild *info = get_child(me, child);
	assert(info, L"No child info!");

	switch (id) {
	case CHILD_PROP_X:
		move(me, info, g_value_get_int(value), info->y, info->w, info->h);
		break;
	case CHILD_PROP_Y:
		move(me, info, info->x, g_value_get_int(value), info->w, info->h);
		break;
	case CHILD_PROP_W:
		move(me, info, info->x, info->y, g_value_get_int(value), info->h);
		break;
	case CHILD_PROP_H:
		move(me, info, info->x, info->y, info->w, g_value_get_int(value));
		break;
	default:
		GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID(here, id, spec);
		break;
	}
}

static void basic_get_property(GtkContainer *here, GtkWidget *child, guint id, GValue *value, GParamSpec *spec) {
	BasicChild *info = get_child(BASIC(here), child);
	assert(info, L"No child info!");

	switch (id) {
	case CHILD_PROP_X:
		g_value_set_int(value, info->x);
		break;
	case CHILD_PROP_Y:
		g_value_set_int(value, info->y);
		break;
	case CHILD_PROP_W:
		g_value_set_int(value, info->w);
		break;
	case CHILD_PROP_H:
		g_value_set_int(value, info->h);
		break;
	default:
		GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID(here, id, spec);
		break;
	}
}

static void basic_preferred_width(GtkWidget *widget, int *minimum, int *natural) {
	Basic *me = BASIC(widget);

	*minimum = 0;
	*natural = 0;

	for (GList *children = me->children; children; children = children->next) {
		BasicChild *child = (BasicChild *)children->data;

		if (!gtk_widget_get_visible(child->widget))
			continue;

		gint child_min, child_nat;
		gtk_widget_get_preferred_width(child->widget, &child_min, &child_nat);

		*minimum = max(*minimum, child->x + child_min);
		*natural = max(*natural, child->x + child_nat);
	}

	// We report a minimum size of zero to allow the window to resize to any size. Any constraints
	// are handled in Storm.
	*minimum = 0;
}

static void basic_preferred_height(GtkWidget *widget, int *minimum, int *natural) {
	Basic *me = BASIC(widget);

	*minimum = 0;
	*natural = 0;

	for (GList *children = me->children; children; children = children->next) {
		BasicChild *child = (BasicChild *)children->data;

		if (!gtk_widget_get_visible(child->widget))
			continue;

		gint child_min, child_nat;
		gtk_widget_get_preferred_height(child->widget, &child_min, &child_nat);

		*minimum = max(*minimum, child->y + child_min);
		*natural = max(*natural, child->y + child_nat);
	}

	// We report a minimum size of zero to allow the window to resize to any size. Any constraints
	// are handled in Storm.
	*minimum = 0;
}

static void basic_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
	Basic *me = BASIC(widget);

	gtk_widget_set_allocation(widget, allocation);
	if (gtk_widget_get_has_window(widget) && gtk_widget_get_realized(widget)) {
		// Seems to never be called, but good to have just in case...
		gdk_window_move_resize(gtk_widget_get_window(widget),
							allocation->x, allocation->y,
							allocation->width, allocation->height);
	}

	for (GList *children = me->children; children; children = children->next) {
		BasicChild *child = (BasicChild *)children->data;

		if (!gtk_widget_get_visible(child->widget))
			continue;

		GtkAllocation alloc;
		alloc.x = allocation->x + child->x;
		alloc.y = allocation->y + child->y;
		alloc.width = child->w;
		alloc.height = child->h;

		gtk_widget_size_allocate(child->widget, &alloc);
	}
}

static void basic_add(GtkContainer *container, GtkWidget *widget) {
	basic_put(BASIC(container), widget, 0, 0, 100, 100);
}

static void basic_remove(GtkContainer *container, GtkWidget *widget) {
	Basic *me = BASIC(container);

	for (GList *children = me->children; children; children = children->next) {
		BasicChild *child = (BasicChild *)children->data;

		if (child->widget == widget) {
			gboolean was_visible = gtk_widget_get_visible(widget);
			gtk_widget_unparent(widget);

			me->children = g_list_remove_link(me->children, children);
			g_list_free(children);
			g_free(child);

			if (was_visible && gtk_widget_get_visible((GtkWidget *)me))
				gtk_widget_queue_resize((GtkWidget *)me);

			break;
		}
	}
}

static void basic_forall(GtkContainer *container, gboolean internals, GtkCallback callback, gpointer data) {
	Basic *me = BASIC(container);

	GList *children = me->children;
	while (children) {
		BasicChild *child = (BasicChild *)children->data;
		children = children->next;

		(*callback)(child->widget, data);
	}
}

static void basic_class_init(BasicClass *klass) {
	GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;
	GtkContainerClass *container_class = (GtkContainerClass *)klass;

	widget_class->get_preferred_width = &basic_preferred_width;
	widget_class->get_preferred_height = &basic_preferred_height;
	widget_class->size_allocate = &basic_size_allocate;

	container_class->add = &basic_add;
	container_class->remove = &basic_remove;
	container_class->forall = &basic_forall;
	container_class->child_type = &basic_child_type;
	container_class->set_child_property = &basic_set_property;
	container_class->get_child_property = &basic_get_property;
	gtk_container_class_handle_border_width(container_class);

	gtk_container_class_install_child_property(container_class,
											CHILD_PROP_X,
											g_param_spec_int("x",
															"X position",
															"X position of child widget",
															G_MININT, G_MAXINT, 0, (GParamFlags)G_PARAM_READWRITE));
	gtk_container_class_install_child_property(container_class,
											CHILD_PROP_Y,
											g_param_spec_int("y",
															"Y position",
															"Y position of child widget",
															G_MININT, G_MAXINT, 0, (GParamFlags)G_PARAM_READWRITE));
	gtk_container_class_install_child_property(container_class,
											CHILD_PROP_W,
											g_param_spec_int("w",
															"Width",
															"Width of child widget",
															0, G_MAXINT, 0, (GParamFlags)G_PARAM_READWRITE));
	gtk_container_class_install_child_property(container_class,
											CHILD_PROP_H,
											g_param_spec_int("h",
															"Height",
															"Height of child widget",
															0, G_MAXINT, 0, (GParamFlags)G_PARAM_READWRITE));
}

static void basic_init(Basic *instance) {
	instance->children = NULL;
	gtk_widget_set_has_window(GTK_WIDGET(instance), FALSE);
}

GtkWidget *basic_new() {
	return (GtkWidget *)g_object_new(TYPE_BASIC, NULL);
}

#endif
