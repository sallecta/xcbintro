 #include <xcb/xcb.h>
 #include <stdio.h>
 #include <stdlib.h>
 
int main(void) {

  xcb_connection_t   *conn;
  const xcb_setup_t  *setup;
  xcb_screen_t       *screen;
  xcb_window_t       window_id;
  uint32_t           prop_name;
  uint32_t           value_mask;
  uint32_t           value_list[2] ; 
  int                    finished;  
  uint32_t           prop_value;
  xcb_generic_event_t *event;
  xcb_gcontext_t       foreground;

	xcb_point_t points[5] = { {11, 24}, {30, 10}, {49, 24}, {42, 46}, {18, 46} }; 
	xcb_segment_t segments[2] = { {60, 20, 90, 40}, {60, 40, 90, 20} };
	xcb_rectangle_t rect = {15, 65, 30, 20};
	xcb_arc_t arc = {60, 70, 30, 20, 0, 180 << 6};

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
  value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  value_list[0] = screen->white_pixel;
  value_list[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | 
                  XCB_EVENT_MASK_KEY_PRESS;

  xcb_create_window(conn, screen->root_depth, 
     window_id, screen->root, 0, 0, 100, 100, 1,
     XCB_WINDOW_CLASS_INPUT_OUTPUT, 
     screen->root_visual, value_mask, value_list);

  /* Display the window */
  xcb_map_window(conn, window_id);
  xcb_flush(conn);

  /* Execute the event loop */
  while (!finished && (event = xcb_wait_for_event(conn))) {

    switch(event->response_type) {

      case XCB_KEY_PRESS:
        printf("Keycode: %d\n", ((xcb_key_press_event_t*)event)->detail);
        finished = 1;
        break;

      case XCB_BUTTON_PRESS:
        printf("Button pressed: %u\n", ((xcb_button_press_event_t*)event)->detail);
        printf("X-coordinate: %u\n", ((xcb_button_press_event_t*)event)->event_x);
        printf("Y-coordinate: %u\n", ((xcb_button_press_event_t*)event)->event_y);
        break;

      case XCB_EXPOSE:

        /* Draw polygon */
        xcb_fill_poly(conn, window_id, foreground, XCB_POLY_SHAPE_CONVEX, 
                      XCB_COORD_MODE_ORIGIN, 5, points);

        /* Draw line segments */
        xcb_poly_segment(conn, window_id, foreground, 2, segments);

        /* Draw rectangle */
        xcb_poly_fill_rectangle(conn, window_id, foreground, 1, &rect);

        /* Draw arc */
        xcb_poly_arc(conn, window_id, foreground, 1, &arc);

        xcb_flush(conn);
        break;

    }
    free(event);
  }

  /* Disconnect from X server */
  xcb_disconnect(conn);
  return 0;
}