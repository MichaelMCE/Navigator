


#include "commonGlue.h"




#if ENABLE_TOUCH_FT5216

extern int gnssReceiver_PassthroughEnabled;
extern application_t inst;

static touch_t touch;
static IntervalTimer touchTimer;

touchCtx_t touchCtx = {0};



void touch_init ()
{
	memset(&touch, 0, sizeof(touch_t));
	//memset(&touchCtx, 0, sizeof(touchCtx_t));

	touch_start(FT5216_INT);
	touchCtx.rotate = TOUCH_ROTATION;
	touchCtx.enabled = TOUCH_REPORTS_ON;
	touchCtx.pressed = 0;
	touchCtx.tready = 1;
	touchCtx.t0 = 0;
}

static void ISR_touch_sig ()
{
	touchCtx.tready = 0xFF;
}

void touch_startTimer ()
{
	touchTimer.begin(ISR_touch_sig, 7*1000);			// 5 == 200hz, 7 = 142hz, in microseconds
	touchTimer.priority(150);
}

static inline void opSendTouch (touchCtx_t *ctx, touch_t *touch, const int isReleased)
{
	if (!isReleased){
		//printf(CS("Touch down: %i %i"), touch->points[0].x, touch->points[0].y);

		if (gnssReceiver_PassthroughEnabled)
			gnssReceiver_PassthroughEnabled = 0;

		if (++inst.renderFlags == 6)
			inst.renderFlags = 0;
		render_signalUpdate();
	}else{
		//printf(CS("Touch released"));
	}
}

void touch_task (touchCtx_t *ctx)
{
	if (ctx->enabled == TOUCH_REPORTS_HALT)
		return;

	int total = touch_process(&touch, ctx->rotate);

	if (!total && ctx->pressed){
		ctx->pressed = 0;
		//printf(CS("\nReleased"));
		
		opSendTouch(ctx, &touch, 1);
	}
	

	if (total && !ctx->pressed){
		ctx->pressed = 1;
		//printf(CS("\nPressed"));

		opSendTouch(ctx, &touch, 0);
	}
}


#endif

