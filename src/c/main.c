#include <pebble.h>
#include "effect_layer.h"

// ---- Platform-specific layout -----------------------------------------------
// Emery: 200x228 (~1.4x scale vs aplite/basalt 144x168)
#ifdef PBL_PLATFORM_EMERY
  // Battery sprite sheet offsets (Battery~emery.png: 28x174, 1.4x scale)
  #define BATT_STRIP_W      28
  #define BATT_STRIP_H      14
  #define BATT_DST_Y        154
  #define BATT_DST_W        17
  #define BATT_DST_H        7
  #define BATT_AMPM_X_AM    18
  #define BATT_AMPM_X_PM    24
  #define BATT_AMPM_W       4
  #define BATT_AMPM_H       7
  #define BATT_BT_Y         161
  #define BATT_BT_W         7
  #define BATT_BT_H         13
  // Layer rects
  #define R_CLOCK           GRect(7,   35,  110, 110)
  #define R_SECS            GRect(116, 35,  80,  72)
  #define R_DDMM            GRect(119, 118, 74,  28)
  #define HHMM_X            14
  #define HHMM_Y            152
  #define HHMM_W_OFFSET     28
  #define HHMM_H            56
  #define R_SS              GRect(141, 158, 50,  42)
  #define R_DST             GRect(169, 149, 17,  7)
  #define R_AMPM            GRect(10,  153, 4,   7)
  #define R_BATT            GRect(164, 204, 28,  14)
  #define R_RADIO           GRect(153, 205, 7,   13)
  #define R_INV_COLOR       GRect(6,   31,  189, 167)
  // Pizza-slice second tick shape (trapezoid, inner=narrow, outer=wide)
  #define SEC_INNER_R     9      // px from center to inner edge of tick
  #define SEC_OUTER_R     35     // px from center to outer edge of tick
  #define SEC_INNER_HW    0.1f   // half-width at inner edge (px)
  #define SEC_OUTER_HW    .71f   // half-width at outer edge (px)
#else
  // Aplite / Basalt: 144x168
  #define BATT_STRIP_W      20
  #define BATT_STRIP_H      10
  #define BATT_DST_Y        110
  #define BATT_DST_W        12
  #define BATT_DST_H        5
  #define BATT_AMPM_X_AM    13
  #define BATT_AMPM_X_PM    17
  #define BATT_AMPM_W       3
  #define BATT_AMPM_H       5
  #define BATT_BT_Y         115
  #define BATT_BT_W         5
  #define BATT_BT_H         9
  // Layer rects
  #define R_CLOCK           GRect(5,  26, 79, 79)
  #define R_SECS            GRect(86, 26, 53, 53)
  #define R_DDMM            GRect(86, 80, 53, 21)
  #define HHMM_X            10
  #define HHMM_Y            96
  #define HHMM_W_OFFSET     20
  #define HHMM_H            41
  #define R_SS              GRect(106, 106, 31, 31)
  #define R_DST             GRect(122, 110, 12, 5)
  #define R_AMPM            GRect(7,   113, 3,  5)
  #define R_BATT            GRect(118, 150, 20, 10)
  #define R_RADIO           GRect(110, 151, 5,  9)
  #define R_INV_COLOR       GRect(4,   23,  136, 123)
#endif
// -----------------------------------------------------------------------------

enum ConfigKeys {
	CONFIG_KEY_INV=1,
	CONFIG_KEY_VIBR=2,
	CONFIG_KEY_DATEFMT=3,
	CONFIG_KEY_SECS=4,
	CONFIG_KEY_VIBR_BT=5,
	CONFIG_KEY_DATEMODE=6,
	CONFIG_KEY_SHOWSEC=7,
	CONFIG_KEY_REDSEC=8,
	CONFIG_KEY_SECSREFRESH=9,
	CONFIG_KEY_SHAKESECS=10,
	CONFIG_KEY_SHAKESECSDUR=11,
	CONFIG_KEY_INDIGLOW_EN=12,
	CONFIG_KEY_INDIGLOW_COLOR=13
};

typedef struct {
	bool inv, datemode;
	bool vibr, vibr_bt;
	bool secs, redsec;
	uint8_t showsec;
	bool datefmt;
	bool shakesecs;
	uint8_t shakesecsdur;
	bool indiglow_en;
	uint32_t indiglow_color;
} CfgDta_t;

static const uint32_t segments[] = {100, 100, 100};
static const VibePattern vibe_pat = {
	.durations = segments,
	.num_segments = ARRAY_LENGTH(segments),
};

static const uint32_t segments_bt[] = {100, 100, 100, 100, 400, 400, 100, 100, 100};
static const VibePattern vibe_pat_bt = {
	.durations = segments_bt,
	.num_segments = ARRAY_LENGTH(segments_bt),
};

Window *window;
Layer *clock_layer, *secs_layer;
TextLayer *ddmm_layer, *hhmm_layer, *ss_layer;
BitmapLayer *background_layer, *battery_layer, *radio_layer, *dst_layer, *ampm_layer;
InverterLayer *inv_layer;

static GBitmap *background, *batteryAll, *batteryAkt;
static GFont digitS, digitM, digitL, digitSS;
static CfgDta_t CfgData;
static bool bIsCharging;
static uint8_t aktBatt, aktBattAnim, aktHH, aktMM, aktSS;
static AppTimer *timer_batt;
static AppTimer *s_shake_timer;
static bool s_shake_active;
static GPath *hour_arrow, *minute_arrow, *second_arrow;

char ddmmBuffer[] = "00-00";
char hhmmBuffer[] = "00:00";
char ssBuffer[] = "00";

#ifdef PBL_PLATFORM_EMERY
// Emery clock layer is 110x110 (radius ~55) vs aplite 79x79 (radius ~39), scale ~1.41
static const GPathInfo HOUR_HAND_POINTS =   { 3, (GPoint []) { { 0, -11 }, { -3, -42 }, { 3, -42 } } };
static const GPathInfo MINUTE_HAND_POINTS = { 4, (GPoint []) { { -3, -47 }, { -3, -52 }, { 3, -52 }, { 3, -47 } } };
// Secs layer is 80x72 (center at 40,36), tip -33 leaves 3px margin
static const GPathInfo SECOND_HAND_POINTS = { 3, (GPoint []) { { 0, -10 }, { -1, -33 }, { 1, -33 } } };
#else
static const GPathInfo HOUR_HAND_POINTS =   { 3, (GPoint []) { { 0, -8 }, { -3, -30 }, { 3, -30 } } };
static const GPathInfo MINUTE_HAND_POINTS = { 4, (GPoint []) { { -3, -33 }, { -3, -40 }, { 3, -40 }, { 3, -33 } } };
static const GPathInfo SECOND_HAND_POINTS = { 3, (GPoint []) { { 0, -7 }, { -1, -26 }, { 1, -26 } } };
#endif

//-----------------------------------------------------------------------------------------------------------------------
char *upcase(char *str) {
    for (int i = 0; str[i] != 0; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 'a' - 'A';
        }
    }
    return str;
}
//-----------------------------------------------------------------------------------------------------------------------
static void timerCallbackBattery(void *data) 
{
	if (bIsCharging)
	{
		int nImage = 10 - (aktBattAnim / 10);
		
		bitmap_layer_set_bitmap(battery_layer, NULL);
		gbitmap_destroy(batteryAkt);
		batteryAkt = gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, BATT_STRIP_H * nImage, BATT_STRIP_W, BATT_STRIP_H));
		bitmap_layer_set_bitmap(battery_layer, batteryAkt);

		aktBattAnim += 10;
		if (aktBattAnim > 100)
			aktBattAnim = aktBatt;
		timer_batt = app_timer_register(1000, timerCallbackBattery, NULL);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
void battery_state_service_handler(BatteryChargeState charge_state) 
{
	int nImage = 0;
	aktBatt = charge_state.charge_percent;
	
	if (charge_state.is_charging)
	{
		if (!bIsCharging)
		{
			nImage = 10;
			bIsCharging = true;
			aktBattAnim = aktBatt;
			timer_batt = app_timer_register(1000, timerCallbackBattery, NULL);
		}
	}
	else
	{
		nImage = 10 - (aktBatt / 10);
		bIsCharging = false;
	}
	
	bitmap_layer_set_bitmap(battery_layer, NULL);
	gbitmap_destroy(batteryAkt);
	batteryAkt = gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, BATT_STRIP_H * nImage, BATT_STRIP_W, BATT_STRIP_H));
	bitmap_layer_set_bitmap(battery_layer, batteryAkt);
}
//-----------------------------------------------------------------------------------------------------------------------
void bluetooth_connection_handler(bool connected)
{
	layer_set_hidden(bitmap_layer_get_layer(radio_layer), connected != true);
	
	if (!connected && CfgData.vibr_bt)
		vibes_enqueue_custom_pattern(vibe_pat_bt); 	
}
//-----------------------------------------------------------------------------------------------------------------------
static void clock_update_proc(Layer *layer, GContext *ctx) 
{
	GRect bounds = layer_get_bounds(layer);
	const GPoint center = grect_center_point(&bounds);
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
#ifdef PBL_PLATFORM_EMERY
	graphics_context_set_stroke_width(ctx, 2);
#endif
	
	gpath_move_to(hour_arrow, center);
	gpath_move_to(minute_arrow, center);

	//Hour Arrow
	gpath_rotate_to(hour_arrow, (TRIG_MAX_ANGLE * (((aktHH % 12) * 6) + (aktMM / 10))) / (12 * 6));
	gpath_draw_filled(ctx, hour_arrow);
	gpath_draw_outline(ctx, hour_arrow);
		
	//Minute = Hour Arrow + Minute Arrow
	gpath_rotate_to(hour_arrow, TRIG_MAX_ANGLE * aktMM / 60);
	gpath_draw_filled(ctx, hour_arrow);
	gpath_draw_outline(ctx, hour_arrow);
	gpath_rotate_to(minute_arrow, TRIG_MAX_ANGLE * aktMM / 60);
	gpath_draw_filled(ctx, minute_arrow);
	gpath_draw_outline(ctx, minute_arrow);
}
//-----------------------------------------------------------------------------------------------------------------------
#ifdef PBL_PLATFORM_EMERY
// Draw one pizza-slice (trapezoid) tick at 'angle'. Color must be set before calling.
static void draw_sec_tick(GContext *ctx, GPoint center, int32_t angle) {
	float fsin = (float)sin_lookup(angle) / TRIG_MAX_RATIO;
	float fcos = (float)cos_lookup(angle) / TRIG_MAX_RATIO;
	int range = SEC_OUTER_R - SEC_INNER_R;
	for (int r = SEC_INNER_R; r <= SEC_OUTER_R; r++) {
		float rx = center.x + fsin * r;
		float ry = center.y - fcos * r;
		float t  = (float)(r - SEC_INNER_R) / range;
		float hw = SEC_INNER_HW + t * (SEC_OUTER_HW - SEC_INNER_HW);
		// Perpendicular to radius (fsin, -fcos) is (fcos, fsin)
		float dx = fcos * hw;
		float dy = fsin * hw;
		GPoint p1 = {(int16_t)(rx - dx + 0.5f), (int16_t)(ry - dy + 0.5f)};
		GPoint p2 = {(int16_t)(rx + dx + 0.5f), (int16_t)(ry + dy + 0.5f)};
		graphics_draw_line(ctx, p1, p2);
	}
}
#endif
//-----------------------------------------------------------------------------------------------------------------------
static void secs_update_proc(Layer *layer, GContext *ctx) 
{
	GRect bounds = layer_get_bounds(layer);
	const GPoint center = grect_center_point(&bounds);
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);

#ifdef PBL_PLATFORM_EMERY
	for (int32_t i = 0; i <= aktSS; i++) {
		int32_t angle = TRIG_MAX_ANGLE * i / 60;
		GColor tick_color = (i == aktSS && CfgData.redsec) ? GColorRed : GColorBlack;
		graphics_context_set_stroke_color(ctx, tick_color);
		graphics_context_set_fill_color(ctx, tick_color);
		draw_sec_tick(ctx, center, angle);
	}
#else
	gpath_move_to(second_arrow, center);
	for (int32_t i=0; i<=aktSS; i++)
	{
		gpath_rotate_to(second_arrow, TRIG_MAX_ANGLE * i / 60);
		gpath_draw_outline(ctx, second_arrow);
	}
#endif	
/*	
	//Other possibility of Drawing the Second Arrow
	GPoint ptSecE, ptSecA;
	const int16_t SecLen = bounds.size.w / 2;

	for (int32_t i=0; i<=aktSS; i++)
	{
		int32_t second_angle = TRIG_MAX_ANGLE * i / 60;
		ptSecE.x = (int16_t)(sin_lookup(second_angle) * (int32_t)SecLen / TRIG_MAX_RATIO) + center.x;
		ptSecE.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)SecLen / TRIG_MAX_RATIO) + center.y;
		ptSecA.x = (int16_t)(sin_lookup(second_angle) * (int32_t)6 / TRIG_MAX_RATIO) + center.x;
		ptSecA.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)6 / TRIG_MAX_RATIO) + center.y;
		graphics_draw_line(ctx, ptSecE, ptSecA);
	}
*/
}
//-----------------------------------------------------------------------------------------------------------------------
static void update_tick_subscription(void);
static void shake_timer_callback(void *data)
{
	s_shake_active = false;
	s_shake_timer = NULL;
	update_tick_subscription();
}
//-----------------------------------------------------------------------------------------------------------------------
static void accel_tap_handler(AccelAxisType axis, int32_t direction)
{
	if (!CfgData.shakesecs) return;
	s_shake_active = true;
	if (s_shake_timer) app_timer_cancel(s_shake_timer);
	if (CfgData.shakesecsdur > 0)
		s_shake_timer = app_timer_register((uint32_t)CfgData.shakesecsdur * 1000, shake_timer_callback, NULL);
	update_tick_subscription();
	// Force an immediate seconds refresh
	time_t temp = time(NULL);
	struct tm *t = localtime(&temp);
	aktSS = t->tm_sec;
	layer_mark_dirty(secs_layer);
	if (!CfgData.datemode)
	{
		strftime(ssBuffer, sizeof(ssBuffer), "%S", t);
		text_layer_set_text(ss_layer, ssBuffer);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	aktHH = tick_time->tm_hour;
	aktMM = tick_time->tm_min;

	if (CfgData.showsec != 0)
	{
		uint8_t refresh = s_shake_active ? 1 : CfgData.showsec;
		bool do_refresh;
		if (refresh == 60)
			do_refresh = (tick_time->tm_sec == 0 || units_changed == MINUTE_UNIT);
		else
			do_refresh = ((tick_time->tm_sec % refresh) == 0 || units_changed == MINUTE_UNIT);

		if (do_refresh)
		{
			aktSS = (refresh == 60) ? 0 : tick_time->tm_sec;
			layer_mark_dirty(secs_layer);

			if (!CfgData.datemode)
			{
				if (refresh == 60)
					strcpy(ssBuffer, "00");
				else
					strftime(ssBuffer, sizeof(ssBuffer), "%S", tick_time);
				text_layer_set_text(ss_layer, ssBuffer);
			}
		}
	}

	if(tick_time->tm_sec == 0 || units_changed == MINUTE_UNIT)
	{
		layer_mark_dirty(clock_layer);

		//strcpy(hhmmBuffer, "88:88");
		//strftime(hhmmBuffer, sizeof(hhmmBuffer), "%S:%S", tick_time);
		
		if (CfgData.datemode)
		{
			strftime(ddmmBuffer, sizeof(ddmmBuffer), "%Y", tick_time);

			strftime(hhmmBuffer, sizeof(hhmmBuffer), 
				CfgData.datefmt ? "%m.%d" : "%d.%m", tick_time);

			char tmp[] = "MON";
			strftime(tmp, sizeof(tmp), "%a", tick_time);
			strncpy(ssBuffer, upcase(tmp), sizeof(ssBuffer));
			ssBuffer[2] = '\0';
			app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Curr Day: [%s]/[%s]", tmp, ssBuffer);
			text_layer_set_text(ss_layer, ssBuffer);
		}
		else
		{
			if(clock_is_24h_style())
				strftime(hhmmBuffer, sizeof(hhmmBuffer), "%H:%M", tick_time);
			else
				strftime(hhmmBuffer, sizeof(hhmmBuffer), "%I:%M", tick_time);
			strftime(ddmmBuffer, sizeof(ddmmBuffer), 
				CfgData.datefmt ? "%m.%d" : "%d.%m", tick_time);
		}
		text_layer_set_text(ddmm_layer, ddmmBuffer);
		text_layer_set_text(hhmm_layer, hhmmBuffer);
		
		//AM/PM Display every 12 hour
		if (((aktHH % 12) == 0 && aktMM == 0) || units_changed == MINUTE_UNIT)
			bitmap_layer_set_bitmap(ampm_layer, gbitmap_create_as_sub_bitmap(batteryAll, GRect(aktHH < 12 ? BATT_AMPM_X_AM : BATT_AMPM_X_PM, BATT_DST_Y, BATT_AMPM_W, BATT_AMPM_H)));
		
		//Check DST at 4h at morning
		if ((aktHH == 4 && aktMM == 0) || units_changed == MINUTE_UNIT)
			layer_set_hidden(bitmap_layer_get_layer(dst_layer), tick_time->tm_isdst != 1);
		
		//Hourly vibrate
		if (CfgData.vibr && tick_time->tm_min == 0)
			vibes_enqueue_custom_pattern(vibe_pat); 	
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void update_tick_subscription(void)
{
	TimeUnits units = (!s_shake_active && CfgData.showsec == 60) ? MINUTE_UNIT : SECOND_UNIT;
	tick_timer_service_subscribe(units, (TickHandler)tick_handler);
}
//-----------------------------------------------------------------------------------------------------------------------
static void update_configuration(void)
{
    if (persist_exists(CONFIG_KEY_INV))
		CfgData.inv = persist_read_bool(CONFIG_KEY_INV);
	else	
		CfgData.inv = false;
	
    if (persist_exists(CONFIG_KEY_DATEMODE))
		CfgData.datemode = persist_read_bool(CONFIG_KEY_DATEMODE);
	else	
		CfgData.datemode = false;
	
    if (persist_exists(CONFIG_KEY_VIBR))
		CfgData.vibr = persist_read_bool(CONFIG_KEY_VIBR);
	else	
		CfgData.vibr = false;
	
    if (persist_exists(CONFIG_KEY_VIBR_BT))
		CfgData.vibr_bt = persist_read_bool(CONFIG_KEY_VIBR_BT);
	else	
		CfgData.vibr_bt = true;
	
    if (persist_exists(CONFIG_KEY_SECS))
		CfgData.secs = persist_read_bool(CONFIG_KEY_SECS);
	else	
		CfgData.secs = true;
	
    if (persist_exists(CONFIG_KEY_SHOWSEC))
		CfgData.showsec = persist_read_int(CONFIG_KEY_SHOWSEC);
	else	
		CfgData.showsec = 1;
	
    if (persist_exists(CONFIG_KEY_DATEFMT))
		CfgData.datefmt = persist_read_bool(CONFIG_KEY_DATEFMT);
	else	
		CfgData.datefmt = false;

    if (persist_exists(CONFIG_KEY_REDSEC))
		CfgData.redsec = persist_read_bool(CONFIG_KEY_REDSEC);
	else
		CfgData.redsec = false;

    if (persist_exists(CONFIG_KEY_SHAKESECS))
		CfgData.shakesecs = persist_read_bool(CONFIG_KEY_SHAKESECS);
	else
		CfgData.shakesecs = false;

    if (persist_exists(CONFIG_KEY_SHAKESECSDUR))
		CfgData.shakesecsdur = (uint8_t)persist_read_int(CONFIG_KEY_SHAKESECSDUR);
	else
		CfgData.shakesecsdur = 10;

    if (persist_exists(CONFIG_KEY_INDIGLOW_EN))
		CfgData.indiglow_en = persist_read_bool(CONFIG_KEY_INDIGLOW_EN);
	else
		CfgData.indiglow_en = false;

    if (persist_exists(CONFIG_KEY_INDIGLOW_COLOR))
		CfgData.indiglow_color = (uint32_t)persist_read_int(CONFIG_KEY_INDIGLOW_COLOR);
	else
		CfgData.indiglow_color = 0x44F841;

	if (CfgData.indiglow_en)
		light_set_color_rgb888(CfgData.indiglow_color);

	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Curr Conf: inv:%d, datemode:%d, vibr:%d, vibr_bt:%d, secs:%d, showsec:%d, datefmt:%d", CfgData.inv, CfgData.datemode, CfgData.vibr, CfgData.vibr_bt, CfgData.secs, CfgData.showsec, CfgData.datefmt);
	
	Layer *window_layer = window_get_root_layer(window);

	//Seconds Hand Layer
	layer_remove_from_parent(secs_layer);
	if (CfgData.showsec != 0)
		layer_add_child(window_layer, secs_layer);
	
	//Seconds Digits Visible?
	layer_remove_from_parent(text_layer_get_layer(ss_layer));
	if ((CfgData.secs && CfgData.showsec != 0) || CfgData.datemode)
	{
		layer_add_child(window_layer, text_layer_get_layer(ss_layer));
		text_layer_set_text_alignment(hhmm_layer, GTextAlignmentLeft);
	}
	else
		text_layer_set_text_alignment(hhmm_layer, GTextAlignmentCenter);

	text_layer_set_text_alignment(ss_layer, CfgData.datemode ? GTextAlignmentRight : GTextAlignmentCenter);
#ifdef PBL_PLATFORM_EMERY
	// Datemode shows a 3-letter day name — needs DSEG14 alphabet support.
	// Normal mode shows digits only — use DSEG7 to match the time font style.
	text_layer_set_font(ss_layer, CfgData.datemode ? digitM : digitSS);
#endif
	
	//AM/PM Layer
	layer_remove_from_parent(bitmap_layer_get_layer(ampm_layer));
	if(!clock_is_24h_style() && !CfgData.datemode)
		layer_add_child(window_layer, bitmap_layer_get_layer(ampm_layer));
	
	//Inverter Layer
	layer_remove_from_parent(inverter_layer_get_layer(inv_layer));
	if (CfgData.inv)
		layer_add_child(window_layer, inverter_layer_get_layer(inv_layer));

	//Shake-to-show-seconds
	if (CfgData.shakesecs)
		accel_tap_service_subscribe(accel_tap_handler);
	else
	{
		accel_tap_service_unsubscribe();
		s_shake_active = false;
		if (s_shake_timer) { app_timer_cancel(s_shake_timer); s_shake_timer = NULL; }
	}
	
	//Get a time structure so that it doesn't start blank
	time_t temp = time(NULL);
	struct tm *t = localtime(&temp);

	//Manually call the tick handler when the window is loading
	tick_handler(t, MINUTE_UNIT);

	//Startup fake shake: show seconds immediately for the chosen duration
	if (CfgData.shakesecsdur > 0)
	{
		s_shake_active = true;
		if (s_shake_timer) app_timer_cancel(s_shake_timer);
		s_shake_timer = app_timer_register((uint32_t)CfgData.shakesecsdur * 1000, shake_timer_callback, NULL);
		update_tick_subscription();
	}

	//Set Battery state
	BatteryChargeState btchg = battery_state_service_peek();
	battery_state_service_handler(btchg);
	
	//Set Bluetooth state
	bool connected = bluetooth_connection_service_peek();
	bluetooth_connection_handler(connected);

	update_tick_subscription();
}
//-----------------------------------------------------------------------------------------------------------------------
void in_received_handler(DictionaryIterator *received, void *ctx)
{
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "enter in_received_handler");
    
	Tuple *akt_tuple = dict_read_first(received);
    while (akt_tuple)
    {
        app_log(APP_LOG_LEVEL_DEBUG,
                __FILE__,
                __LINE__,
                "KEY %d=%s", (int16_t)akt_tuple->key,
                akt_tuple->value->cstring);

		if (akt_tuple->key == CONFIG_KEY_INV)
			persist_write_bool(CONFIG_KEY_INV, akt_tuple->value->int32 != 0);

		if (akt_tuple->key == CONFIG_KEY_DATEMODE)
			persist_write_bool(CONFIG_KEY_DATEMODE, akt_tuple->value->int32 != 0);

		if (akt_tuple->key == CONFIG_KEY_VIBR)
			persist_write_bool(CONFIG_KEY_VIBR, akt_tuple->value->int32 != 0);

		if (akt_tuple->key == CONFIG_KEY_VIBR_BT)
			persist_write_bool(CONFIG_KEY_VIBR_BT, akt_tuple->value->int32 != 0);

		if (akt_tuple->key == CONFIG_KEY_SECS)
			persist_write_bool(CONFIG_KEY_SECS, akt_tuple->value->int32 != 0);

		if (akt_tuple->key == CONFIG_KEY_SHOWSEC)
			persist_write_int(CONFIG_KEY_SHOWSEC, 
				strcmp(akt_tuple->value->cstring, "nev") == 0 ? 0 : 
				strcmp(akt_tuple->value->cstring, "05s") == 0 ? 5 : 
				strcmp(akt_tuple->value->cstring, "10s") == 0 ? 10 : 
				strcmp(akt_tuple->value->cstring, "15s") == 0 ? 15 : 
				strcmp(akt_tuple->value->cstring, "30s") == 0 ? 30 :
				strcmp(akt_tuple->value->cstring, "60s") == 0 ? 60 : 1);

		if (akt_tuple->key == CONFIG_KEY_DATEFMT)
			persist_write_bool(CONFIG_KEY_DATEFMT, akt_tuple->value->int32 != 0);

		if (akt_tuple->key == CONFIG_KEY_REDSEC)
			persist_write_bool(CONFIG_KEY_REDSEC, akt_tuple->value->int32 != 0);

		if (akt_tuple->key == CONFIG_KEY_SHAKESECS)
			persist_write_bool(CONFIG_KEY_SHAKESECS, akt_tuple->value->int32 != 0);

		if (akt_tuple->key == CONFIG_KEY_SHAKESECSDUR)
			persist_write_int(CONFIG_KEY_SHAKESECSDUR, akt_tuple->value->int32);

		if (akt_tuple->key == CONFIG_KEY_INDIGLOW_EN)
			persist_write_bool(CONFIG_KEY_INDIGLOW_EN, akt_tuple->value->int32 != 0);

		if (akt_tuple->key == CONFIG_KEY_INDIGLOW_COLOR)
			persist_write_int(CONFIG_KEY_INDIGLOW_COLOR, akt_tuple->value->int32);
		
		akt_tuple = dict_read_next(received);
	}
	
    update_configuration();
}
//-----------------------------------------------------------------------------------------------------------------------
void in_dropped_handler(AppMessageResult reason, void *ctx)
{
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "Message dropped, reason code %d",
            reason);
}
//-----------------------------------------------------------------------------------------------------------------------
void window_load(Window *window)
{
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	//Init Background
	background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	background_layer = bitmap_layer_create(bounds);
	bitmap_layer_set_background_color(background_layer, GColorClear);
	bitmap_layer_set_bitmap(background_layer, background);
	layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));
	
	//Init Clock Layer
	clock_layer = layer_create(R_CLOCK);
	layer_set_update_proc(clock_layer, clock_update_proc);
	hour_arrow = gpath_create(&HOUR_HAND_POINTS);
	minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
	layer_add_child(window_layer, clock_layer);	
	
	//Init Seconds Layer
	secs_layer = layer_create(R_SECS);
	layer_set_update_proc(secs_layer, secs_update_proc);
	second_arrow = gpath_create(&SECOND_HAND_POINTS);
	layer_add_child(window_layer, secs_layer);	
	
	digitS = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_20));
	digitM = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_30));
	digitL = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_40));
#ifdef PBL_PLATFORM_EMERY
	fonts_unload_custom_font(digitS);
	fonts_unload_custom_font(digitM);
	fonts_unload_custom_font(digitL);
	digitS = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DSEG_18));
	digitM = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DSEG_27_27));
	digitSS = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DSEG_27S_27));
	digitL = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DSEG_33));
#endif
	batteryAll = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UTILITIES);

	//DAY+MONTH/YEAR layer
	ddmm_layer = text_layer_create(R_DDMM);
	text_layer_set_background_color(ddmm_layer, GColorClear);
	text_layer_set_text_color(ddmm_layer, GColorBlack);
	text_layer_set_text_alignment(ddmm_layer, GTextAlignmentCenter);
	text_layer_set_font(ddmm_layer, digitS);
	layer_add_child(window_layer, text_layer_get_layer(ddmm_layer));
      
	//HOUR+MINUTE/DAY+MONTH layer
	hhmm_layer = text_layer_create(GRect(HHMM_X, HHMM_Y, bounds.size.w - HHMM_W_OFFSET, HHMM_H));
	text_layer_set_background_color(hhmm_layer, GColorClear);
	text_layer_set_text_color(hhmm_layer, GColorBlack);
	text_layer_set_text_alignment(hhmm_layer, GTextAlignmentLeft);
	text_layer_set_font(hhmm_layer, digitL);
	layer_add_child(window_layer, text_layer_get_layer(hhmm_layer));
	
	//SECOND Digits/Weekday layer
	ss_layer = text_layer_create(R_SS);
	text_layer_set_background_color(ss_layer, GColorClear);
	text_layer_set_text_color(ss_layer, GColorBlack);
	text_layer_set_text_alignment(ss_layer, GTextAlignmentCenter);
	text_layer_set_font(ss_layer, digitSS);
	layer_add_child(window_layer, text_layer_get_layer(ss_layer));
     
	//DST layer
	dst_layer = bitmap_layer_create(R_DST);
	bitmap_layer_set_background_color(dst_layer, GColorClear);
	bitmap_layer_set_bitmap(dst_layer, gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, BATT_DST_Y, BATT_DST_W, BATT_DST_H)));
	layer_add_child(window_layer, bitmap_layer_get_layer(dst_layer));
      
	//AM/PM layer
	ampm_layer = bitmap_layer_create(R_AMPM);
	bitmap_layer_set_background_color(ampm_layer, GColorClear);
      
	//Init battery
	battery_layer = bitmap_layer_create(R_BATT); 
	bitmap_layer_set_background_color(battery_layer, GColorClear);
	layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));
	
	//Init bluetooth radio
	radio_layer = bitmap_layer_create(R_RADIO);
	bitmap_layer_set_background_color(radio_layer, GColorClear);
	bitmap_layer_set_bitmap(radio_layer, gbitmap_create_as_sub_bitmap(batteryAll, GRect(0, BATT_BT_Y, BATT_BT_W, BATT_BT_H)));
	layer_add_child(window_layer, bitmap_layer_get_layer(radio_layer));
	
	//Init Inverter Layer
#ifdef PBL_COLOR
	inv_layer = inverter_layer_create(R_INV_COLOR);
#else
	inv_layer = inverter_layer_create(bounds);	
#endif	
	//Update Configuration
	update_configuration();
}
//-----------------------------------------------------------------------------------------------------------------------
void window_unload(Window *window)
{
	//Destroy Layers
	layer_destroy(clock_layer);
	layer_destroy(secs_layer);
	
	//Destroy GPaths
	gpath_destroy(hour_arrow);
	gpath_destroy(minute_arrow);
	gpath_destroy(second_arrow);
	
	//Destroy text layers
	text_layer_destroy(ddmm_layer);
	text_layer_destroy(hhmm_layer);
	text_layer_destroy(ss_layer);
	
	//Unload Fonts
	fonts_unload_custom_font(digitS);
	fonts_unload_custom_font(digitM);
	fonts_unload_custom_font(digitL);
	
	//Destroy GBitmaps
	gbitmap_destroy(batteryAkt);
	gbitmap_destroy(batteryAll);
	gbitmap_destroy(background);

	//Destroy BitmapLayers
	bitmap_layer_destroy(dst_layer);
	bitmap_layer_destroy(ampm_layer);
	bitmap_layer_destroy(battery_layer);
	bitmap_layer_destroy(radio_layer);
	bitmap_layer_destroy(background_layer);
	
	//Destroy Inverter Layer
	inverter_layer_destroy(inv_layer);
}
//-----------------------------------------------------------------------------------------------------------------------
void handle_init(void) 
{
	char* sLocale = setlocale(LC_TIME, ""), sLang[3];
	if (strncmp(sLocale, "en", 2) == 0)
		strcpy(sLang, "en");
	else if (strncmp(sLocale, "de", 2) == 0)
		strcpy(sLang, "de");
	else if (strncmp(sLocale, "es", 2) == 0)
		strcpy(sLang, "es");
	else if (strncmp(sLocale, "fr", 2) == 0)
		strcpy(sLang, "fr");
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Time locale is set to: %s/%s", sLocale, sLang);

	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
    window_stack_push(window, true);

	//Subscribe services
	update_tick_subscription();
	battery_state_service_subscribe(&battery_state_service_handler);
	bluetooth_connection_service_subscribe(&bluetooth_connection_handler);
	
	//Subscribe messages
	app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_open(512, 64);
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
}
//-----------------------------------------------------------------------------------------------------------------------
void handle_deinit(void) 
{
	app_timer_cancel(timer_batt);
	if (s_shake_timer) app_timer_cancel(s_shake_timer);
	accel_tap_service_unsubscribe();
	app_message_deregister_callbacks();
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
	window_destroy(window);
}
//-----------------------------------------------------------------------------------------------------------------------
int main(void) 
{
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}
//-----------------------------------------------------------------------------------------------------------------------
