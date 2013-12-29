/*

   "Classic" Digital Watch Pebble App

 */

// Standard includes
#include "pebble.h"

// App-specific data
// Sanglass Outline
#define SG_OFFSET_X	(30)
#define SG_OFFSET_Y	(10)
#define SG_W	(144-SG_OFFSET_X*2)
#define SG_H	(168-16-SG_OFFSET_Y*2)
#define SG_SLIT_W	(4)
#define	SG_HH	(40)
#define	SG_GH	((SG_H-SG_HH*2)/2)
#define SG_GW	((SG_W-SG_SLIT_W)/2)
#define	SG_OUTLINE_GAP	(4)
#define	SG_OUTLINE_WIDTH	(4)
#define SG_AMOUNT	(SG_W*SG_HH*0.8 + SG_GW*SG_GH)
	
Window *window; // All apps must have at least one window
TextLayer *time_layer; // The clock
static BitmapLayer *sand_flow = NULL;
	
static GPath *sandglass_outline_ptr = NULL;
static GPath *sandglass_inline_ptr = NULL;
static GPath *sand_top_outline_ptr = NULL;
static GPath *sand_bottom_outline_ptr = NULL;
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
static const GPathInfo SANDGLASS_INLINE = {
	.num_points = 12,
	.points = (GPoint[]) {
		{SG_OFFSET_X+SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_OUTLINE_WIDTH}, 
		{SG_OFFSET_X+SG_W-SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_OUTLINE_WIDTH},
		{SG_OFFSET_X+SG_W-SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_HH-SG_OUTLINE_WIDTH}, 
		{SG_OFFSET_X+SG_W-SG_GW-SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_HH+SG_GH-SG_OUTLINE_WIDTH},
		{SG_OFFSET_X+SG_W-SG_GW-SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_HH+SG_GH+SG_OUTLINE_WIDTH},
		{SG_OFFSET_X+SG_W-SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_HH+SG_GH*2+SG_OUTLINE_WIDTH}, 
		{SG_OFFSET_X+SG_W-SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_H-SG_OUTLINE_WIDTH},
		{SG_OFFSET_X+SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_H-SG_OUTLINE_WIDTH}, 
		{SG_OFFSET_X+SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_HH+SG_GH*2+SG_OUTLINE_WIDTH},
		{SG_OFFSET_X+SG_GW+SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_HH+SG_GH+SG_OUTLINE_WIDTH}, 
		{SG_OFFSET_X+SG_GW+SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_HH+SG_GH-SG_OUTLINE_WIDTH}, 
		{SG_OFFSET_X+SG_OUTLINE_WIDTH, SG_OFFSET_Y+SG_HH-SG_OUTLINE_WIDTH}
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

// Called once per second
static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
	static char time_text[] = "00:00:00"; // Needs to be static because it's used by the system later.

	strftime(time_text, sizeof(time_text), "%T", tick_time);
	text_layer_set_text(time_layer, "");
}

static void accel_data_handler(AccelData *data, uint32_t num_samples) {
}

static int get_triangle_ratio(const int amount) {
	int a;
	
	for(a=0; a<100; a++) {
		if( amount < (SG_GW*a/100*SG_GH*a/100) )
			break;
	}
	
	return a;
}

static void draw_outline_w(GContext* ctx, GPath* gpath_ptr, int w, int r) {
	for(unsigned int i=0; i < gpath_ptr->num_points-1; i++) {
		/*graphics_fill_rect(
			ctx, 
			(GRect){{(gpath_ptr->points[i].x)-w/2, (gpath_ptr->points[i].y)-w/2},
			{(gpath_ptr->points[i+1].x)+w/2, (gpath_ptr->points[i+1].y)+w/2}}, 
			r, GCornersAll
		);*/
		for(int j=-w/2; j < w/2; j++) {
			graphics_draw_line(
				ctx, 
				(GPoint){gpath_ptr->points[i].x+j, gpath_ptr->points[i].y+j},
				(GPoint){gpath_ptr->points[i+1].x+j, gpath_ptr->points[i+1].y+j} 
		    );	
		}
	}
}
static void draw_sand(GContext* ctx, struct tm* t) {
	int sand_flowed_amount = SG_AMOUNT*(t->tm_min*60 + t->tm_sec)/3600;
	int sand_stay_amount = SG_AMOUNT - sand_flowed_amount;
	
	// top
	if( sand_stay_amount < SG_GW*SG_GH ) {
		int a = get_triangle_ratio(sand_stay_amount);
		int w = SG_GW*a/100;
		int h = SG_GH*a/100;
		sand_top_outline.points[0] = (GPoint){SG_GW, SG_HH+SG_GH-SG_OUTLINE_GAP};
		sand_top_outline.points[1] = (GPoint){SG_GW-w+SG_OUTLINE_GAP, SG_HH+SG_GH-h};
		sand_top_outline.points[2] = (GPoint){SG_GW+SG_SLIT_W+w-SG_OUTLINE_GAP, SG_HH+SG_GH-h};
		sand_top_outline.points[3] = (GPoint){SG_GW+SG_SLIT_W, SG_HH+SG_GH-SG_OUTLINE_GAP};
		sand_top_outline.points[4] = sand_top_outline.points[3];
		sand_top_outline.points[5] = sand_top_outline.points[3];
		sand_top_outline.points[6] = sand_top_outline.points[3];
		sand_top_outline.points[7] = sand_top_outline.points[3];
	} else {
		int h = (sand_stay_amount-SG_GW*SG_GH)/SG_W;		
		sand_top_outline.points[0] = (GPoint){SG_GW, SG_HH+SG_GH-SG_OUTLINE_GAP};
		sand_top_outline.points[1] = (GPoint){SG_OUTLINE_GAP, SG_HH-0};
		sand_top_outline.points[2] = (GPoint){SG_OUTLINE_GAP, SG_HH-h};
		sand_top_outline.points[3] = (GPoint){SG_W-SG_OUTLINE_GAP, SG_HH-h};
		sand_top_outline.points[4] = (GPoint){SG_W-SG_OUTLINE_GAP, SG_HH-0};
		sand_top_outline.points[5] = (GPoint){SG_GW+SG_SLIT_W, SG_HH+SG_GH-SG_OUTLINE_GAP};
		sand_top_outline.points[6] = sand_top_outline.points[5];
		sand_top_outline.points[7] = sand_top_outline.points[5];
	}
	gpath_draw_filled(ctx, sand_top_outline_ptr);
	gpath_move_to(sand_top_outline_ptr, (GPoint){SG_OFFSET_X, SG_OFFSET_Y});
	
	// bottom
	if( sand_flowed_amount < SG_GW*SG_GH ) {
		int a = get_triangle_ratio(sand_flowed_amount);
		int w = SG_GW*a/100;
		int h = SG_GH*a/100;
		sand_bottom_outline.points[0] = (GPoint){SG_GW+SG_SLIT_W, SG_H-h+SG_OUTLINE_GAP};
		sand_bottom_outline.points[1] = (GPoint){SG_GW+SG_SLIT_W+w-SG_OUTLINE_GAP, SG_H-SG_OUTLINE_GAP};
		sand_bottom_outline.points[2] = (GPoint){SG_GW-w+SG_OUTLINE_GAP, SG_H-SG_OUTLINE_GAP};
		sand_bottom_outline.points[3] = (GPoint){SG_GW, SG_H-h+SG_OUTLINE_GAP};
		sand_bottom_outline.points[4] = sand_bottom_outline.points[3];
		sand_bottom_outline.points[5] = sand_bottom_outline.points[3];
		sand_bottom_outline.points[6] = sand_bottom_outline.points[3];
		sand_bottom_outline.points[7] = sand_bottom_outline.points[3];
	} else {
		int h = (sand_flowed_amount-SG_GW*SG_GH)/SG_W;		
		sand_bottom_outline.points[0] = (GPoint){SG_GW+SG_SLIT_W, SG_H-h-SG_GH+SG_OUTLINE_GAP};
		sand_bottom_outline.points[1] = (GPoint){SG_W-SG_OUTLINE_GAP, SG_H-h+0};
		sand_bottom_outline.points[2] = (GPoint){SG_W-SG_OUTLINE_GAP, SG_H-SG_OUTLINE_GAP};
		sand_bottom_outline.points[3] = (GPoint){SG_OUTLINE_GAP, SG_H-SG_OUTLINE_GAP};
		sand_bottom_outline.points[4] = (GPoint){SG_OUTLINE_GAP, SG_H-h+0};
		sand_bottom_outline.points[5] = (GPoint){SG_GW, SG_H-h-SG_GH+SG_OUTLINE_GAP};
		sand_bottom_outline.points[6] = sand_bottom_outline.points[5];
		sand_bottom_outline.points[7] = sand_bottom_outline.points[5];
	}
	gpath_draw_filled(ctx, sand_bottom_outline_ptr);
	gpath_move_to(sand_bottom_outline_ptr, (GPoint){SG_OFFSET_X, SG_OFFSET_Y});
}

// Called whien layer updates
static void draw_sandglass_outline(Layer *my_layer, GContext* ctx) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	
	// Stroke the outline of sandglass:
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	gpath_draw_outline(ctx, sandglass_outline_ptr);
	gpath_move_to(sandglass_outline_ptr, (GPoint){SG_OFFSET_X, SG_OFFSET_Y});
	//graphics_context_set_stroke_color(ctx, GColorBlack);
	//graphics_context_set_fill_color(ctx, GColorBlack);
	//gpath_draw_outline(ctx, sandglass_inline_ptr);
	//draw_outline_w(ctx, sandglass_outline_ptr, 2, 2);
	//
	
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	draw_sand(ctx, t);
	
	// draw sand flow
	for(int i=SG_OFFSET_Y+SG_HH+SG_GH-SG_OUTLINE_GAP; i < SG_OFFSET_Y+SG_H-SG_OUTLINE_GAP; i++) {
		if( i%2==0 ) {
			graphics_draw_pixel(ctx, (GPoint){72, i+t->tm_sec%2});
		} else {
			graphics_draw_pixel(ctx, (GPoint){73, i+t->tm_sec%2});
		}
	}
}

static void setup_sandglass_outline(void) {
	sandglass_outline_ptr = gpath_create(&SANDGLASS_OUTLINE);
	sandglass_inline_ptr = gpath_create(&SANDGLASS_INLINE);
	sand_top_outline_ptr = gpath_create(&sand_top_outline);
	sand_bottom_outline_ptr = gpath_create(&sand_bottom_outline);
	
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
	time_layer = text_layer_create(GRect(29, 54, 144-40 /* width */, 168-54 /* height */));
	text_layer_set_text_color(time_layer, GColorBlack);
	text_layer_set_background_color(time_layer, GColorClear);
	text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));

//	// Ensures time is displayed immediately (will break if NULL tick event accessed).
//	// (This is why it's a good idea to have a separate routine to do the update itself.)
	time_t now = time(NULL);
	struct tm *current_time = localtime(&now);
	handle_second_tick(current_time, SECOND_UNIT);
	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);

 	//
	image_layer = layer_create(GRect(0, 0, 144, 168));
	setup_sandglass_outline();
	layer_set_update_proc(image_layer, &draw_sandglass_outline);
	
	accel_data_service_subscribe(25, &accel_data_handler);
	layer_add_child(window_get_root_layer(window), image_layer);
 	layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
}


static void do_deinit(void) {
	text_layer_destroy(time_layer);
	layer_destroy(image_layer);
	gpath_destroy(sandglass_outline_ptr);
	gpath_destroy(sand_top_outline_ptr);
	gpath_destroy(sand_bottom_outline_ptr);
	window_destroy(window);
}

// The main event/run loop for our app
int main(void) {
	do_init();
	app_event_loop();
	do_deinit();
}
