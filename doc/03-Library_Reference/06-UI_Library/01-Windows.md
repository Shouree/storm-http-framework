Windows
=======

The window management part of the UI library is built around *windows*, *containers*, *frames*, and
*dialogs*. In the context of the UI library, a *window* is a rectangle on the screen. It is
typically embedded inside another window to represent some component, such as a button. A
*container* is a window that may contain other child windows. A *frame* is a container that has a
border, and can be used as a top-level window. As such, the root of a window hierarchy is typically
a *frame*, not a window. Finally, a *dialog* is a frame that might have a parent frame (i.e. it is
displayed "on top of" another window).

The terminology above follows the naming in the Windows API (which is what the UI library was
initially developed for). Gtk uses other terms. Gtk uses the term *widget* for what the UI library
calls a *window*.

Each of these central classes are documented in more detail below. This page of the manual then
concludes with an overview of the different UI components.

Threading
---------

All operations on windows happen on the [stormname:ui.Ui] thread. All components described in this
section are thereby actors that are tied to this thread.

The [stormname:ui.Ui] thread is in charge of interfacing with the operating system. As such, it is a
good idea to not block the `Ui` thread if possible. In Basic Storm, this is fairly easy by using the
`spawn` keyword to create another thread. It is generally sufficient if the thread that called a
callback function returns to the caller quickly, even if another UThread is spawned on the same OS
thread. However, care should be taken not to execute heavy computations on the `Ui` thread without
periodically calling `yield` to let the UI library process messages from the operating system.
Computationally expensive operations are therefore typically better to delegate to other OS threads
entirely.


Usage
-----

The UI library is designed with the intent that the core classes (i.e. `Window`, `Container`,
`Frame`, and `Dialog`) should be subclassed to create custom windows. As such, most events from the
windowing system are notified in the form of overloaded functions. Other components (e.g. `Button`)
are not designed to be subclassed, and instead provide callbacks in the form of function pointers.
This means that creating a custom window or dialog typically involves creating a subclass to `Frame`
or `Dialog` as desired, and let the subclass create all child components, and listen for callbacks
from them using callbacks from function pointers.


Creation of Windows
-------------------

Due to restrictions in the underlying APIs, it is not always possible to create individual windows
without first creating their parents. In order to not restrict the UI library needlessly, the UI
library does not impose such restrictions (e.g. it is possible to create a `Button` without first
creating a parent `Window`). This does, however, mean that the UI library creates the actual windows
lazily, whenever the classes are attached to a parent.

In most cases, this is transparent to the user. It does, however, mean that some features (such as
focus management) may not be fully functional until the windows have actually been created (Gtk uses
the term *realized* to describe this). It also means that some care needs to be taken for top-level
windows, since they do not have a parent. The UI library thus requires top-level windows to be
explicitly created by calling the `create` member. This creates the entire window in one operation,
and has the benefit that it lets the underlying system present the window as a unit, and avoid
partially drawn windows as far as possible.

This model also means that the system can clean up any resources associated with the created windows
as soon as the top-level window is closed. It is therefore not necessary to explicitly destroy
windows, as long as the top-level window is closed.


Window
------

As mentioned above, a [stormname:ui.Window] is a rectangular region on the screen. By itself it
represents a window that has to be embedded inside another window. However, the subclass
[stormname:ui.Frame] lifts this restriction.

Coordinates in a window are always relative to the top-left edge of the window. The exception is the
coordinates returned by `pos`, which are relative to the window's parent.

The `Window` class has the following members:

```stormdoc
@ui.Window
- .__init()
- .parent()
- .rootFrame()
- .visible(*)
- .enabled(*)
- .text(*)
- .pos(*)
- .minSize()
- .font(*)
- .focus()
- .update()
- .repaint()
- .resized(*)
- .painter(*)
- .setTimer(*)
- .clearTimer()
```

The `Window` class also has a number of callbacks that are called when certain events occur. These
are member functions that can be overloaded by a subclass. Many of the functions return a boolean.
This indicates if the handler recognized the event, and signals to the UI library that the event
should not be processed further (e.g. by parent windows).

```stormdoc
@ui.Window
- .onKey(*)
- .onChar(*)
- .onClick(*)
- .onDblClick(*)
- .onMouseMove(*)
- .onMouseEnter()
- .onMouseLeave()
- .onMouseVScroll(*)
- .onMouseHScroll(*)
- .onTimer()
```


Container
---------

The [stormname:ui.Container] class is a subclass of the `Window` class and represents a window that
may contain child windows. To achieve this, the `Container` class provides the following additional
functions:

```stormdoc
@ui.Container
- .add(*)
- .remove(*)
```


Frame
-----

The [stormname:ui.Frame] class represents a top-level window that can be a parent of a window
hierarchy. It inherits from the `Container` class and may therefore contain sub-windows. Since the
frame does not have a parent, it needs to be created manually. This is achieved with the `create`
function.

The `Frame` provides the following member functions:

```stormdoc
@ui.Frame
- .__init(Str)
- .__init(Str, core.geometry.Size)
- .create()
- .close()
- .waitForClose()
- .size(*)
- .fullscreen(*)
- .cursorVisible(*)
- .accelerators()
- .menu(*)
- .popupMenu(*)
```


Dialog
------

The class [stormname:ui.Dialog] class represents a frame that acts as a dialog. This means two
things: first, a dialog may have a default behavior for the enter and escape keys (typically OK and
Cancel respectively). Secondly, the dialog may be a modal dialog that blocks access to another
window.

It provides the following members:

```stormdoc
@ui.Dialog
- .__init(Str)
- .__init(Str, core.geometry.Size)
- .show(*)
- .close(*)
- .defaultChoice(*)
- .onDestroy(*)
```


Other Components
----------------

The UI library provides a number of UI components as follows:

- [stormname:ui.Label]

  A simple piece of text displayed in the window.

- [stormname:ui.Edit]

  A simple box that allows simple text editing.

- [stormname:ui.Button]

  A button that can be pressed by the user.

- [stormname:ui.CheckButton]

  A checkmark that can be selected or deselected.

- [stormname:ui.RadioButton]

  A button that can be associated with a group of similar buttons. Only allows one button in the
  group to be selected at any one time.

- [stormname:ui.ListView]

  A component that shows a scrollable list. The list may contain columns with different contents.

- [stormname:ui.ScrollWindow]

  Provides a viewport to another window, where the user can scroll the contents of the other window.

- [stormname:ui.TabView]

  Provides a set of tabs, where the user can select one out of many possible views.


Additionally, the UI library provides the following convenience interfaces:

- [stormname:ui.FilePicker]

  Provides access to the operating system's default open/save dialogs.

- [stormname:ui.MessageDialog]

  Default message dialogs. Use any of the following functions to easily create message dialogs:

  ```stormdoc
  @ui
  - .showMessage(*)
  - .showYesNoQuestion(*)
  - .showYesNoCancelQuestion(*)
  ```

- [stormname:full:ui.showLicenseDialog(ui.Frame, ui.ProgramInfo)]

  Show a default dialog that contains the licenses of all currently used libraries. Useful to
  conveniently comply with requirements in license agreements for GUI applications.
