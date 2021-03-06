#include <pebble.h>
#include "skipwin.h"
#include "common.h"
#include "commonwin.h"

#define LEN_DATE 12

static SkipSetCallBack s_set_event;
static time_t s_skip_until;
static char *s_date;
static bool s_show_noskip;
  
static Window *s_window;
static GBitmap *s_res_img_upaction;
static GBitmap *s_res_img_okaction;
static GBitmap *s_res_img_downaction;
static GFont s_res_gothic_24;
static GFont s_res_gothic_28;
static ActionBarLayer *s_actionbarlayer;
static Layer *s_info_layer;

static void draw_info(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer); 
  
  graphics_context_set_text_color(ctx, GColorWhite);
  // Draw title
  graphics_draw_text(ctx, "Skip Until", s_res_gothic_24, GRect(2, (bounds.size.h/2)-55, bounds.size.w-4, 32), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  // Draw date
  graphics_draw_text(ctx, s_date, s_res_gothic_28, GRect(2, (bounds.size.h/2)-18, bounds.size.w-4, 32), 
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw "No Skipping"
  if (s_show_noskip)
    graphics_draw_text(ctx, "(No skipping)", s_res_gothic_24, GRect(7, (bounds.size.h/2)+18, bounds.size.w-14, 32), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void initialise_ui(void) {
  s_date = malloc(LEN_DATE);
  
  GRect bounds;
  Layer *root_layer = NULL;
  s_window = window_create_fullscreen(&root_layer, &bounds);
  
  s_res_img_upaction = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UPACTION2);
  s_res_img_okaction = gbitmap_create_with_resource(RESOURCE_ID_IMG_OKACTION);
  s_res_img_downaction = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWNACTION2);
  s_res_gothic_24 = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  s_res_gothic_28 = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  // s_actionbarlayer
  s_actionbarlayer = actionbar_create(s_window, root_layer, &bounds, s_res_img_upaction, s_res_img_okaction, s_res_img_downaction);
  
  // info layer
  s_info_layer = layer_create_with_proc(root_layer, draw_info,
                                        GRect(0, 0, bounds.size.w-ACTION_BAR_WIDTH, bounds.size.h));
}

static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(s_actionbarlayer);
  layer_destroy(s_info_layer);
  gbitmap_destroy(s_res_img_upaction);
  gbitmap_destroy(s_res_img_okaction);
  gbitmap_destroy(s_res_img_downaction);
  
  free(s_date);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

static time_t get_today() {
  return strip_time(time(NULL) + get_UTC_offset(NULL));
}

static void update_date_display() {
  time_t skip_utc = s_skip_until - get_UTC_offset(NULL);
  struct tm *t = localtime(&skip_utc);
  strftime(s_date, LEN_DATE, "%a, %b %d", t);
  s_show_noskip = (s_skip_until <= get_today());
  layer_mark_dirty(s_info_layer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Close this screen
  hide_skipwin();
  // Pass skip date back (skip date of today or earlier disables skip)
  s_set_event((s_skip_until <= get_today()) ? 0 : s_skip_until);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  time_t today = get_today();
  
  if (s_skip_until <= today)
      // If the skip date was already set to today, wrap around to 4 weeks from now
      s_skip_until = today + (28 * 24 * 60 * 60);
  else
      // Set the date to the previous date
      s_skip_until -= (24 * 60 * 60);
  
  update_date_display();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  time_t today = get_today();
  
  if (day_diff(today, s_skip_until) >= 28)
    // If the skip date is already set to 4 weeks from today, wrap around to today
    s_skip_until = today;
  else
    // Set the date to the next date
    s_skip_until += (24 * 60 * 60);
  
  update_date_display();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, down_click_handler);
}

void show_skipwin(time_t skip_until, SkipSetCallBack set_event) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  
  // Store pointer to callback function
  s_set_event = set_event;
  
  // Trap action bar clicks
  window_set_click_config_provider(s_window, click_config_provider);
  
  time_t today = get_today();
  
  // Validate and store current skip date
  if (skip_until == 0)
    s_skip_until = today;
  else if (skip_until < today)
    s_skip_until = today;
  else
    s_skip_until = strip_time(skip_until);
  
  update_date_display();
  
  window_stack_push(s_window, true);
}

void hide_skipwin(void) {
  window_stack_remove(s_window, true);
}
