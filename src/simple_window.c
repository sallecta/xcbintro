#include <xcb/xcb.h>
 
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