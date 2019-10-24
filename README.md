# xcbintro
An Introduction to XCB Programming by Matt Scarpino 

[Based on codeproject.com article](https://www.codeproject.com/Articles/1089819/An-Introduction-to-XCB-Programming).

Athor: [Matt Scarpino](https://www.codeproject.com/script/Membership/View.aspx?mid=10170939)

[The Code Project Open License (CPOL) 1.02](http://www.codeproject.com/info/cpol10.aspx)

# An Introduction to XCB Programming
Developing Low-Level Linux Applications with XCB (X protocol C-language Binding)

## Introduction

Years ago, I wrote graphical user interfaces (GUIs) for various platforms. For a Linux application, I tried to work with the X Window System library, commonly called Xlib. I was stunned by the amount of code required to perform simple tasks, and I decided to use a different toolset.

Today, developers can develop low-level GUIs for the X Window System using XCB, which stands for X protocol C-language Binding. An XCB program can perform the same operations as one coded with Xlib, but requires substantially less code. In addition, XCB provides "latency hiding, direct access to the protocol, improved threading support, and extensibility."

The goal of this article is to explain the basics of XCB application development. There are three central topics: creating a window, handling events, and drawing graphics. For each topic, I've provided a source file that demonstrates how the functions and data structures can be used in practice.

## 1. Preliminary Requirements

Understanding the content of this article requires a basic familiarity with C programming. To compile the example code, you need to have the XCB library (libxcb.so) and header file (xcb.h) installed on your development system.

## 2. The X Window System

Though Wayland and Mir are growing in popularity, the X Window System (also called X Windows or just X) remains the most common framework for managing windows on Linux computers. Originally developed by MIT in the 1980s, it has become widely adopted on UNIX and Linux computers, and it can be accessed on Windows computers through Cygwin. The goal of XCB and Xlib is to enable programmers to access the X Window System in their applications.

When dealing with X, the first thing to understand is the difference between displays, screens, and windows. According to the official documentation, a display is a "collection of monitors that share a common keyboard and pointer." A screen identifies a physical monitor and a window is a graphical element drawn on a monitor. It's important to understand that a display can have multiple screens and each screen can have multiple windows.

The second thing to understand about X is its use of client-server communication. X supports remote displays, and given the usual client-server model, you might expect that the user's system receives display information from a remote X server. But that's not how X works. The X server runs on the user's computer and receives display information from client applications. In most cases, the client and server run on the same system.

For each screen, the X server manages a window that occupies the entire area. This is called the root window. When client applications connect to the server, the server allows them to create child windows. If a user performs an action while a child window has focus, the X server will provide the client application with information about the user's action. This information is provided in the form of an event.

Each resource managed by an X server has an identifier. Each display has a name and each screen has a number. Before an application can create a new window or resource, it needs to obtain a suitable ID.

## 3. Creating a Window

Most XCB applications start by performing three fundamental tasks:

    Connect to the X Server
    Access a screen
    Create and display a window in the screen

The following discussion explains how these tasks can be accomplished. The last part of the section presents an application that displays a simple window for five seconds.

### 3.1  Connect to the X Server

Whether you program in Xlib or XCB, an application's first step usually involves connecting to the X server. In XCB, the function to use is xcb_connect:

xcb_connection_t* xcb_connect(const char *display, int *screen);

The first argument identifies the X server's display name. If this is set to NULL, the function will use the value of the DISPLAY environment variable.

The second argument points to a number for the screen that should be connected. If this is set to NULL, the function will set the screen number to 0. This represents the first available screen.

The xcb_connection_t structure is opaque, so there's no way to know whether the connection was created successfully. For this reason, XCB provides the function xcb_connection_has_error, whose declaration is given as follows:

int xcb_connection_has_error(xcb_connection_t *c);

This accepts the xcb_connection_t* returned by xcb_connect and returns a value that identifies if the connection was established. An error will result in a non-zero value.

The following code shows how these two functions can be used in practice:

xcb_connection_t *conn;

conn = xcb_connect(NULL, NULL);

if (xcb_connection_has_error(conn)) {
  printf("Error opening display.\n");
  exit(1);
}

This code creates a connection to the X server whose name is given by the DISPLAY environment variable. The screen number is 0.


### 3.2  Access the Screen

Before an application can create a window, it needs to access a screen. To do this, the first step is to access properties of the X server and its display environment. This is made possible by xcb_get_setup:

const xcb_setup_t* xcb_get_setup(xcb_connection_t *c);

The return value is an xcb_setup_t, which provides a number of useful fields, including the following:

    roots_len — the number of root windows managed by the X server
    bitmap_format_scanline_unit — the number of bits in a scanline unit
    bitmap_format_scanline_pad — the number of bits used to pad each scanline
    bitmap_format_bit_order — identifies whether the leftmost bit in the screen is the least significant bit or most significant bit
    protocol_major_version — major version of the supported X Window System protocol
    protocol_minor_version — minor version or the supported X Window System protocol

This xcb_setup_t is important because it allows us to access the X server's screens. This access is made possible by another function called xcb_setup_roots_iterator:

xcb_screen_iterator_t xcb_setup_roots_iterator(xcb_setup_t *set);

The xcb_screen_iterator_t structure has a data field of type xcb_screen_t*, which represents a screen. The fields of this structure include the following:

    root — the root window ID
    root_depth — bits per pixel in the screen
    root_visual — pointer to a xcb_visualid_t structure containing the screen's color mapping
    width_in_pixels — screen width in pixels
    height_in_pixels — screen height in pixels
    width_in_millimeters — screen width in millimeters
    height_in_millimeters — screen height in millimeters
    black_pixel — value corresponding to the screen's black pixel
    white_pixel — value corresponding to the screen's white

The following code shows how these structures are used. It obtains an xcb_setup_t structure from an existing connection (conn), accesses the first screen, and prints the screen's dimensions in pixels:

const xcb_setup_t* setup;
xcb_screen_t* screen;

setup = xcb_get_setup(conn);
screen = xcb_setup_roots_iterator(setup).data;
printf("Screen dimensions: %d, %d\n", screen->width_in_pixels, screen->height_in_pixels);


Obtaining the screen is necessary to create the application's window. The following discussion explains how this can be done.

### 3.3  Create and Display the Window

Now that we've accessed the screen, the next step is to create a window. The main function to use is xcb_create_window:

```c
xcb_create_window(xcb_connection_t *conn,          // Connection to X server
                  uint8_t           depth,         // Screen depth
                  xcb_window_t      window_id,     // ID of the window
                  xcb_window_t      parent_id,     // ID of the parent window
                  int16_t           x,             // Top-left x-coordinate
                  int16_t           y,             // Top-left y-coordinate
                  uint16_t          width,         // Window width in pixels
                  uint16_t          height,        // Window height in pixels
                  uint16_t          border_width,  // Window border width in pixels
                  uint16_t          class,
                  xcb_visualid_t    visual,
                  uint32_t          value_mask,
                  const uint32_t   *value_list);
```

Most of the function's arguments are straightforward. window_id is a unique identifier for the window, and it can be obtained by calling xcb_generate_id, whose signature is as follows:
Hide   Copy Code

uint32_t xcb_generate_id(xcb_connection_t* conn)

parent_id can be set to the root field of the xcb_screen_t structure discussed earlier. Similarly, the visual argument can be set to the screen's root_visual field.

The function's class argument is set to a value of the xcb_window_class_t enumerated type. This is undocumented, but it's usually safe to set class equal to XCB_WINDOW_CLASS_INPUT_OUTPUT.

The value_mask and value_list arguments are related. value_mask identifies an OR'ed series of property names and value_list contains their values. The property identifiers are taken from the xcb_cw_t type, whose values can be found here. One important property is XCB_CW_BACK_PIXEL, which sets the window's background color.

After creating a window, xcb_map_window must be called to make it visible. The signature of this function is given as:
Hide   Copy Code

xcb_void_cookie_t xcb_map_window(xcb_connection_t *conn, xcb_window_t window) 

After calling xcb_map_window, it's common to call xcb_flush to force the window request to be sent to the server. The code in the following discussion demonstrates how this is used.

3.4  Example - Simple Window

The code in the simple_window.c file creates and displays a simple 100x100 pixel window. The main function is given as follows:

int main(void) {

  xcb_connection_t   *conn;
  const xcb_setup_t  *setup;
  xcb_screen_t       *screen;
  xcb_window_t       window_id;
  uint32_t           prop_name;
  uint32_t           prop_value;

  /* Connect to X server */
  conn = xcb_connect(NULL, NULL);
  if(xcb_connection_has_error(conn)) {
    printf("Error opening display.\n");
    exit(1);
  }

  /* Obtain setup info and access the screen */
  setup = xcb_get_setup(conn);
  screen = xcb_setup_roots_iterator(setup).data;

  /* Create window */
  window_id = xcb_generate_id(conn);
  prop_name = XCB_CW_BACK_PIXEL;
  prop_value = screen->white_pixel;

  xcb_create_window(conn, screen->root_depth, 
     window_id, screen->root, 0, 0, 100, 100, 1,
     XCB_WINDOW_CLASS_INPUT_OUTPUT, 
     screen->root_visual, prop_name, &prop_value);

  /* Display the window */
  xcb_map_window(conn, window_id);
  xcb_flush(conn);

  /* Wait for 5 seconds */
  sleep(5);

  /* Disconnect from X server */
  xcb_disconnect(conn);
  return 0;
}


There are at least three interesting points to notice about this code:

    The window's position is (0, 0) and its size is 100x100 pixels. This is configured by the arguments in xcb_create_window.
    The window's background color is set to white by associating the XCB_CW_BACK_PIXEL property with the value screen->white_pixel.
    This code doesn't explicitly close the window. Instead, it calls xcb_disconnect to terminate the connection to the X server.

If XCB is installed on the development system, the source file can be compiled with the following command:

gcc -o simple_window simple_window.c -lxcb

Unless the user clicks the window's close button, the window will stay open for five seconds because of the sleep function. Without this function, the window will close immediately. Rather than call sleep, most XCB applications have an event loop. The following discussion explains how events work.

