#include "pebble.h"
#ifdef PBL_PLATFORM_BASALT // Only use this for 3.0+
  #include "gcolor_definitions.h" // Allows the use of colors
#endif

static Window *s_main_window; // Full window
  
static Layer *s_date_layer, *s_hands_layer; // Date and hands layers
static BitmapLayer *s_background_layer; // Background layer
static GBitmap *s_background_bitmap; // Background bitmap
static TextLayer *s_date_label; // Date label

static char s_hour_buffer[3]; // Hour buffer
static char s_minute_buffer[3]; // Minute buffer
static char s_date_buffer[] = "XXX XXX XX"; // Date buffer

static GFont *s_date_font; // Date font

// Makes text uppercase when called
char *upcase(char *str)
{
    for (int i = 0; str[i] != 0; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 0x20;
        }
    }

    return str;
}

// Updates hands and time when called
static void hands_update_proc(Layer *layer, GContext *ctx) {
    
  time_t now = time(NULL); // Get time
  struct tm *t = localtime(&now); // Create time structure
  
  GRect bounds = layer_get_bounds(layer); // Set bounds to full window
  GPoint center = grect_center_point(&bounds); // Find center

  int32_t minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60; //  Create minute path
  int minY = -56 * cos_lookup(minute_angle) / TRIG_MAX_RATIO + center.y; // Get the Y position
  int minX = 56 * sin_lookup(minute_angle) / TRIG_MAX_RATIO + center.x; // Get the X position
  
  //drawing circles with time
  if (!clock_is_24h_style()) {
    if( t->tm_hour > 11 ) { 
      if( t->tm_hour > 12 ) t->tm_hour -= 12;
    } else {
      if( t->tm_hour == 0 ) t->tm_hour = 12;
    }        
  } 
  
  strftime(s_hour_buffer, sizeof(s_hour_buffer), "%H", t); // Set hour_buffer to hour
  strftime(s_minute_buffer, sizeof(s_minute_buffer), "%M", t); // Set minute_buffer to minute

  graphics_context_set_text_color(ctx, GColorWhite); // Set text color to white
  
  graphics_draw_text(ctx, s_minute_buffer, 
                     fonts_load_custom_font(resource_get_handle(RESOURCE_ID_OPEN_SANS_20)), 
                     GRect(minX - 20, minY -15, 40, 40), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL); // Draw the minute

  graphics_draw_text(ctx, s_hour_buffer, 
                     fonts_load_custom_font(resource_get_handle(RESOURCE_ID_OPEN_SANS_50)), 
                     GRect(center.x - 30, center.y - 31, 60, 60), GTextOverflowModeTrailingEllipsis, 
                     GTextAlignmentCenter, NULL); // Draw the hour
  
}

// Update date when called
static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL); // Get time
  struct tm *t = localtime(&now); // Create time structure
  
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %d", t); // Set date buffer to date DDD MMM dd
  
  upcase(s_date_buffer); // Set date to uppercase
  
  text_layer_set_text(s_date_label, s_date_buffer); // Set label to buffer
}

// Update time when called
static void timer_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_main_window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window); // Create main layer
  GRect bounds = layer_get_bounds(window_layer); // Set bounds for main layer to full screen
  
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_OPEN_SANS_15)); // Create date font
  
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND); // Set the background face bitmap resource
  
  s_background_layer = bitmap_layer_create(bounds); // Create face layer
  s_date_layer = layer_create(bounds); // Create date layer
  s_hands_layer = layer_create(bounds); // Create hands layer
  
  layer_set_update_proc(s_date_layer, date_update_proc); // Update date layer
  layer_set_update_proc(s_hands_layer, hands_update_proc); // Update hands layer
  
  s_date_label = text_layer_create(GRect(0, 149, 144, 90)); // Create day label
  text_layer_set_text_color(s_date_label, GColorWhite); // Set black and white for date number label
  text_layer_set_background_color(s_date_label, GColorClear); // Set background to transparent
  text_layer_set_font(s_date_label, s_date_font); // Set day font
  text_layer_set_text_alignment(s_date_label, GTextAlignmentCenter); // Center day label
  
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap); // Apply face bitmap to face layer
  
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer)); // Add background to main layer
  layer_add_child(window_layer, s_hands_layer); // Add hands layer to main layer
  layer_add_child(window_layer, s_date_layer); // Add date layer to main layer
  layer_add_child(s_date_layer, text_layer_get_layer(s_date_label)); // Add date label to to date layer
}

static void window_unload(Window *window) {
  layer_destroy(s_hands_layer); // Destroy hands layer
  layer_destroy(s_date_layer); // Destroy date layer
  
  bitmap_layer_destroy(s_background_layer); // Destroy face layer
  gbitmap_destroy(s_background_bitmap); // Destroy face bitmap
  
  text_layer_destroy(s_date_label); // Destroy day label 
  
  fonts_unload_custom_font(s_date_font); // Unload date font
}

static void init(void) {
  s_main_window = window_create(); // Create main window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load, // Allow window_load to manage main window
    .unload = window_unload, // Allow window_unload to manage main window
  });
  window_stack_push(s_main_window, true); // Show window. Animated=true
  
  s_hour_buffer[0]  = '\0'; // Reset hour buffer
  s_minute_buffer[0]  = '\0'; // Reset minute buffer
  s_date_buffer[0] = '\0'; // Reset day buffer

  tick_timer_service_subscribe(MINUTE_UNIT, timer_tick); // Update time each minute
}

static void deinit(void) {
  tick_timer_service_unsubscribe(); // Unsubscribe from tick timer
  
  window_destroy(s_main_window); // Destroy main window
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}