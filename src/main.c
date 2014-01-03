/*

   "Classic" Digital Watch Pebble App

 */

// Standard includes
#include "pebble.h"

// App-specific data
// Sanglass Outline
#define SG_OFFSET_X	(30)
#define SG_OFFSET_Y	(20)
#define SG_W	(144-SG_OFFSET_X*2)
#define SG_H	(168-SG_OFFSET_Y*2)
#define SG_SLIT_W	(4)
#define	SG_HH	(40)
#define	SG_GH	((SG_H-SG_HH*2)/2)
#define SG_GW	((SG_W-SG_SLIT_W)/2)
#define	SG_OUTLINE_GAP	(6)
#define	SG_OUTLINE_WIDTH	(4)
#define SG_AMOUNT	( \
					  (SG_W-SG_OUTLINE_GAP*2)*(SG_HH-SG_OUTLINE_GAP)*0.8 + \
					  (SG_W-SG_OUTLINE_GAP*2+SG_SLIT_W)*(SG_GH-SG_OUTLINE_GAP)/2 \
					)
	
Window *window; // All apps must have at least one window
TextLayer *time_layer; // The clock
static BitmapLayer *sand_flow = NULL;
static int amount_at_shake = 0;

static GPath *sandglass_outline_ptr = NULL;
//static GPath *sandglass_inline_ptr = NULL;
static GPath *sand_top_outline_ptr = NULL;
static GPath *sand_bottom_outline_ptr = NULL;
static GPath *one_line_ptr = NULL;
static Layer *image_layer;
static PropertyAnimation *property_animation;

static const GPathInfo SANDGLASS_OUTLINE = {
	.num_points = 10,
	.points = (GPoint[]) {
		{0, 0}, {SG_W, 0},
		{SG_W, SG_HH}, {SG_W-SG_GW, SG_HH+SG_GH},
		{SG_W, SG_HH+SG_GH*2}, {SG_W, SG_H},
		{0, SG_H}, {0, SG_HH+SG_GH*2},
		{SG_GW, SG_HH+SG_GH}, {0, SG_HH}
	}
};

static GPathInfo sand_top_outline = {
	.num_points = 8,
	.points = (GPoint[]) {
		{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}
	}
};

static GPathInfo sand_bottom_outline = {
	.num_points = 8,
	.points = (GPoint[]) {
		{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}
	}
};

static GPathInfo one_line = {
	.num_points = 4,
	.points = (GPoint[]) {
		{0,0}, {0,0}, {0,0}, {0,0}
	}
};

// Called once per second
static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
	static char time_text[] = "00:00:00"; // Needs to be static because it's used by the system later.

	strftime(time_text, sizeof(time_text), "%T", tick_time);
	//text_layer_set_text(time_layer, time_text);
	text_layer_set_text(time_layer, "");
	
	if( tick_time->tm_min==0 && tick_time->tm_sec==0 )
		amount_at_shake = 0;
	//layer_mark_dirty(image_layer);
}

// Called once per second
static void handle_hour_tick(struct tm* tick_time, TimeUnits units_changed) {
	amount_at_shake = 0;
}

static void accel_data_handler(AccelAxisType axis, int32_t direction) {
	time_t now = time(NULL);
	struct tm* t = localtime(&now);
	
	if( axis == ACCEL_AXIS_Z ) {
		amount_at_shake = SG_AMOUNT*(t->tm_min*60 + t->tm_sec)/3600;
		text_layer_set_text(time_layer, "tap");
	}
}

static int get_triangle_ratio(const int amount) {
	int a;
	
	for(a=0; a<100; a++) {
		if( amount < (SG_SLIT_W*2+2*SG_GW*a/100)*(SG_GH*a/100)/2 )
			break;
	}
	
	return a;
}

static int get_round(const int v) {
	if( v < 0 ) {
		return v-TRIG_MAX_RATIO/2;
	} else if( v >= 0) {
		return v+TRIG_MAX_RATIO/2;
	} else {
		return v;
	}
}

static void my_gpath_draw_thickoutline(GContext* ctx, GPath* gpath_ptr, GPoint offset, int w) {
	int w1 = (w+1)/2;
	int w2 = (int)(w/2);
	
	for(unsigned int i=0; i < gpath_ptr->num_points; i++) {
		int grad_deg = atan2_lookup(
			gpath_ptr->points[(i+1)%gpath_ptr->num_points].y - gpath_ptr->points[i].y, 
			gpath_ptr->points[(i+1)%gpath_ptr->num_points].x - gpath_ptr->points[i].x
		);
		one_line_ptr->points[0].x = gpath_ptr->points[i].x + get_round(-sin_lookup(grad_deg)*w1)/TRIG_MAX_RATIO;
		one_line_ptr->points[0].y = gpath_ptr->points[i].y + get_round(cos_lookup(grad_deg)*w2)/TRIG_MAX_RATIO;
		one_line_ptr->points[1].x = gpath_ptr->points[i].x + get_round(sin_lookup(grad_deg)*w2)/TRIG_MAX_RATIO;
		one_line_ptr->points[1].y = gpath_ptr->points[i].y + get_round(-cos_lookup(grad_deg)*w1)/TRIG_MAX_RATIO;

		one_line_ptr->points[2].x = gpath_ptr->points[(i+1)%gpath_ptr->num_points].x 
			+ get_round(sin_lookup(grad_deg)*w1)/TRIG_MAX_RATIO;
		one_line_ptr->points[2].y = gpath_ptr->points[(i+1)%gpath_ptr->num_points].y 
			+ get_round(-cos_lookup(grad_deg)*w2)/TRIG_MAX_RATIO;
		one_line_ptr->points[3].x = gpath_ptr->points[(i+1)%gpath_ptr->num_points].x 
			+ get_round(-sin_lookup(grad_deg)*w2)/TRIG_MAX_RATIO;
		one_line_ptr->points[3].y = gpath_ptr->points[(i+1)%gpath_ptr->num_points].y 
			+ get_round(cos_lookup(grad_deg)*w1)/TRIG_MAX_RATIO;
		
		gpath_move_to(one_line_ptr, offset);
		gpath_draw_filled(ctx, one_line_ptr);
	}
}

static void draw_sand(GContext* ctx, struct tm* t) {
	int sand_flowed_amount = SG_AMOUNT*(t->tm_min*60 + t->tm_sec)/3600;
	int sand_stay_amount = SG_AMOUNT - sand_flowed_amount;
	int trapezoid_amount = (SG_W-SG_OUTLINE_GAP*2+SG_SLIT_W)*(SG_GH-SG_OUTLINE_GAP)/2;
	
	// top
	if( sand_stay_amount < trapezoid_amount ) {
		int a = get_triangle_ratio(sand_stay_amount);
		int w = SG_GW*a/100;
		int h = SG_GH*a/100;
		sand_top_outline.points[0] = (GPoint){SG_GW, SG_HH+SG_GH-SG_OUTLINE_GAP};
		sand_top_outline.points[1] = (GPoint){SG_GW-w, SG_HH+SG_GH-h-SG_OUTLINE_GAP};
		sand_top_outline.points[2] = (GPoint){SG_GW+SG_SLIT_W+w, SG_HH+SG_GH-h-SG_OUTLINE_GAP};
		sand_top_outline.points[3] = (GPoint){SG_GW+SG_SLIT_W, SG_HH+SG_GH-SG_OUTLINE_GAP};
		sand_top_outline.points[4] = sand_top_outline.points[3];
		sand_top_outline.points[5] = sand_top_outline.points[3];
		sand_top_outline.points[6] = sand_top_outline.points[3];
		sand_top_outline.points[7] = sand_top_outline.points[3];
	} else {
		int h = (sand_stay_amount - trapezoid_amount)/(SG_W-SG_OUTLINE_GAP*2);		
		sand_top_outline.points[0] = (GPoint){SG_GW, SG_HH+SG_GH-SG_OUTLINE_GAP};
		sand_top_outline.points[1] = (GPoint){SG_OUTLINE_GAP, SG_HH-0};
		sand_top_outline.points[2] = (GPoint){SG_OUTLINE_GAP, SG_HH-h};
		sand_top_outline.points[3] = (GPoint){SG_W-SG_OUTLINE_GAP, SG_HH-h};
		sand_top_outline.points[4] = (GPoint){SG_W-SG_OUTLINE_GAP, SG_HH-0};
		sand_top_outline.points[5] = (GPoint){SG_GW+SG_SLIT_W, SG_HH+SG_GH-SG_OUTLINE_GAP};
		sand_top_outline.points[6] = sand_top_outline.points[5];
		sand_top_outline.points[7] = sand_top_outline.points[5];
	}
	gpath_move_to(sand_top_outline_ptr, (GPoint){SG_OFFSET_X, SG_OFFSET_Y});
	gpath_draw_filled(ctx, sand_top_outline_ptr);
	
	// bottom
	if( amount_at_shake > 0 ) {
		int sh = amount_at_shake/(SG_W-SG_OUTLINE_GAP*2);
		sh = sh > SG_HH - SG_OUTLINE_GAP ? SG_HH - SG_OUTLINE_GAP : sh;
		int a = get_triangle_ratio(sand_flowed_amount - sh*(SG_W-SG_OUTLINE_GAP*2));
		int w = SG_GW*a/100;
		int h = SG_GH*a/100;
		sand_bottom_outline.points[0] = (GPoint){SG_GW+SG_SLIT_W, SG_H-sh-h-SG_OUTLINE_GAP};
		sand_bottom_outline.points[1] = (GPoint){sand_bottom_outline.points[0].x + w, sand_bottom_outline.points[0].y + h};
		sand_bottom_outline.points[2] = (GPoint){SG_W-SG_OUTLINE_GAP, sand_bottom_outline.points[1].y};
		sand_bottom_outline.points[3] = (GPoint){sand_bottom_outline.points[2].x, sand_bottom_outline.points[2].y + sh};
		sand_bottom_outline.points[4] = (GPoint){SG_OUTLINE_GAP, sand_bottom_outline.points[3].y};
		sand_bottom_outline.points[5] = (GPoint){SG_OUTLINE_GAP, sand_bottom_outline.points[4].y - sh};
		sand_bottom_outline.points[6] = (GPoint){SG_GW - w, sand_bottom_outline.points[5].y};
		sand_bottom_outline.points[7] = (GPoint){SG_GW, sand_bottom_outline.points[6].y - h};
	} else if( sand_flowed_amount < trapezoid_amount ) {
		int a = get_triangle_ratio(sand_flowed_amount);
		int w = SG_GW*a/100;
		int h = SG_GH*a/100;
		sand_bottom_outline.points[0] = (GPoint){SG_GW+SG_SLIT_W, SG_H-h-SG_OUTLINE_GAP};
		sand_bottom_outline.points[1] = (GPoint){SG_GW+SG_SLIT_W+w, SG_H-SG_OUTLINE_GAP};
		sand_bottom_outline.points[2] = (GPoint){SG_GW-w, SG_H-SG_OUTLINE_GAP};
		sand_bottom_outline.points[3] = (GPoint){SG_GW, SG_H-h-SG_OUTLINE_GAP};
		sand_bottom_outline.points[4] = sand_bottom_outline.points[3];
		sand_bottom_outline.points[5] = sand_bottom_outline.points[3];
		sand_bottom_outline.points[6] = sand_bottom_outline.points[3];
		sand_bottom_outline.points[7] = sand_bottom_outline.points[3];
	} else {
		int h = (sand_flowed_amount - trapezoid_amount)/(SG_W-SG_OUTLINE_GAP*2);
		sand_bottom_outline.points[0] = (GPoint){SG_GW+SG_SLIT_W, SG_H-SG_OUTLINE_GAP-h-SG_GH+SG_OUTLINE_GAP};
		sand_bottom_outline.points[1] = (GPoint){SG_W-SG_OUTLINE_GAP, SG_H-SG_OUTLINE_GAP-h};
		sand_bottom_outline.points[2] = (GPoint){SG_W-SG_OUTLINE_GAP, SG_H-SG_OUTLINE_GAP};
		sand_bottom_outline.points[3] = (GPoint){SG_OUTLINE_GAP, SG_H-SG_OUTLINE_GAP};
		sand_bottom_outline.points[4] = (GPoint){SG_OUTLINE_GAP, SG_H-SG_OUTLINE_GAP-h};
		sand_bottom_outline.points[5] = (GPoint){SG_GW, SG_H-SG_OUTLINE_GAP-h-SG_GH+SG_OUTLINE_GAP};
		sand_bottom_outline.points[6] = sand_bottom_outline.points[5];
		sand_bottom_outline.points[7] = sand_bottom_outline.points[5];
	}
	gpath_move_to(sand_bottom_outline_ptr, (GPoint){SG_OFFSET_X, SG_OFFSET_Y});
	gpath_draw_filled(ctx, sand_bottom_outline_ptr);
}

// Called whien layer updates
static void draw_sandglass_outline(Layer *my_layer, GContext* ctx) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	
	// Stroke the outline of sandglass:
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	gpath_move_to(sandglass_outline_ptr, (GPoint){SG_OFFSET_X, SG_OFFSET_Y});
	//gpath_draw_outline(ctx, sandglass_outline_ptr);
	//graphics_context_set_stroke_color(ctx, GColorBlack);
	//graphics_context_set_fill_color(ctx, GColorBlack);
	//gpath_draw_outline(ctx, sandglass_inline_ptr);
	my_gpath_draw_thickoutline(ctx, sandglass_outline_ptr, (GPoint){SG_OFFSET_X,SG_OFFSET_Y}, 8);
	//
	
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	draw_sand(ctx, t);
	
	// draw sand flow
	for(int i=SG_OFFSET_Y+SG_HH+SG_GH-SG_OUTLINE_GAP; i < SG_OFFSET_Y+SG_H-SG_OUTLINE_GAP; i++) {
		if( i%4 < 2 ) {
			graphics_draw_pixel(ctx, (GPoint){72, i+t->tm_sec%2});
		} else {
			graphics_draw_pixel(ctx, (GPoint){73, i+t->tm_sec%2});
		}
	}
}

static void setup_sandglass_outline(void) {
	sandglass_outline_ptr = gpath_create(&SANDGLASS_OUTLINE);
	//sandglass_inline_ptr = gpath_create(&SANDGLASS_INLINE);
	sand_top_outline_ptr = gpath_create(&sand_top_outline);
	sand_bottom_outline_ptr = gpath_create(&sand_bottom_outline);
	one_line_ptr = gpath_create(&one_line);
	
	// Rotate 15 degrees:
	gpath_rotate_to(sandglass_outline_ptr, TRIG_MAX_ANGLE / 360 * 0);
	// Translate by (5, 5):
	//gpath_move_to(sandglass_outline_ptr, GPoint(0, 0));	
}

// Handle the start-up of the app
static void do_init(void) {
	// Create our app's base window
	window = window_create();
	window_stack_push(window, true);
	window_set_background_color(window, GColorWhite);

//	// Init the text layer used to show the time
	time_layer = text_layer_create(GRect(20, 54, 144-40 /* width */, 168-54 /* height */));
	text_layer_set_text_color(time_layer, GColorBlack);
	text_layer_set_background_color(time_layer, GColorClear);
	text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));

//	// Ensures time is displayed immediately (will break if NULL tick event accessed).
//	// (This is why it's a good idea to have a separate routine to do the update itself.)
	time_t now = time(NULL);
	struct tm *current_time = localtime(&now);
	handle_second_tick(current_time, SECOND_UNIT);
	//handle_hour_tick(current_time, HOUR_UNIT);
	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
	//tick_timer_service_subscribe(HOUR_UNIT, &handle_hour_tick);

 	//
	image_layer = layer_create(GRect(0, 0, 144, 168));
	setup_sandglass_outline();
	layer_set_update_proc(image_layer, &draw_sandglass_outline);
	
	accel_tap_service_subscribe(&accel_data_handler);
	
	layer_add_child(window_get_root_layer(window), image_layer);
 	layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
}


static void do_deinit(void) {
	text_layer_destroy(time_layer);
	layer_destroy(image_layer);
	gpath_destroy(sandglass_outline_ptr);
	gpath_destroy(sand_top_outline_ptr);
	gpath_destroy(sand_bottom_outline_ptr);
	gpath_destroy(one_line_ptr);
	window_destroy(window);
}

// The main event/run loop for our app
int main(void) {
	do_init();
	app_event_loop();
	do_deinit();
}
