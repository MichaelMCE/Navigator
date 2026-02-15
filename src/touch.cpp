


#include "commonGlue.h"
#include "FT5216/FT5216.h"




#if ENABLE_TOUCH_FT5216

extern int gnssReceiver_PassthroughEnabled;
extern application_t inst;


static touch_t touchIn;
static touch_t touchDown;
static touch_t touchUp;

touchCtx_t touchCtx = {0};
touch_t touchDebug;
static IntervalTimer touchTimer;




void ISR_touch_sig ()
{
	touchCtx.tready = 0xFF;//touch_process(&touchIn, touchCtx.rotate);
}

void touch_startTimer ()
{
	touchTimer.begin(ISR_touch_sig, 5*1000);			// 5 == 200hz, 7 = 142hz, in microseconds
	touchTimer.priority(150);
}

void touch_init ()
{
	memset(&touchDown, 0, sizeof(touch_t));
	memset(&touchUp, 0, sizeof(touch_t));
	memset(&touchIn, 0, sizeof(touch_t));


	touch_start(FT5216_INT);
	touchCtx.rotate = TOUCH_ROTATION;
	touchCtx.enabled = TOUCH_REPORTS_ON;
	touchCtx.pressed = 0;
	touchCtx.tready = 1;
	touchCtx.t0 = 0;
	touchCtx.touchDragTotal = 0;
	
	touch_startTimer();
}

static inline void opSendTouch (touchCtx_t *ctx, touch_t *touch, const uint32_t state)
{
	if (state == TOUCH_UP){
		//printf(CS("Touch released"));
		
		if (gnssReceiver_PassthroughEnabled){	// shouldn't be here
			gnssReceiver_PassthroughEnabled = 0;
			render_signalUpdate();
			return;
		}

		touchDebug = *touch;
		//render_signalUpdate();
		uiInput(touch->x, touch->y, TOUCH_UP);
		render_signalUpdate();
		
	}else if (state == TOUCH_DOWN){
		//printf(CS("Touch down: %i %i"), touch->points[0].x, touch->points[0].y);
		//uiInput(touch->x, touch->y, TOUCH_DOWN);
		//render_signalUpdate();
	
	}else if (state == TOUCH_MOVE){	// drag
		if ((touchCtx.touchDragTotal&31) == 31)
			render_signalUpdate();
	}
}

void touch_task (touchCtx_t *ctx)
{
	if (ctx->enabled == TOUCH_REPORTS_HALT)
		return;

	touch_t *touch = &touchIn;

	int total = touch_process(touch, ctx->rotate);
	if (total)
		powersaveDisable();
	else
		powersaveEnable();

	if (ctx->pressed){
		int x = touch->x;
		int y = touch->y;

		//printf(CS("Touch %d: %i, %i: %i, %i"), (int)(millis()-touchDown.time), touch->idx, touch->tPoints, x, y);
		touchCtx.touchCord[touchCtx.touchDragTotal].x = x;
		touchCtx.touchCord[touchCtx.touchDragTotal].y = y;
		if (touchCtx.touchDragTotal < TOUCH_BINSIZE-2) touchCtx.touchDragTotal++;
		
		opSendTouch(ctx, touch, TOUCH_MOVE);
		//printf(CS("Touch move : %i %i: %i, %i"), total, touch->tPoints, x, y);
	}

	if (!total && ctx->pressed){
		ctx->pressed = 0;
		//printf(CS("\nReleased"));
		touch->x = touch->points[0].x;
		touch->y = touch->points[0].y;
		touchUp = *touch;

#if 0		
		printf(CS("Touch up: %i, %i"), touchUp.x, touchUp.y);
		printf(CS("Touch points: %i"), (int)touchCtx.touchDragTotal);
		printf(CS("Touch time: %i"), (int)(touchUp.time - touchDown.time));
		int x = pow((touchUp.x - touchDown.x), 2);
		int y = pow((touchUp.y - touchDown.y), 2);
		float dist = sqrtf(x + y);
		printf(CS("Touch dist: %.2f"), dist);
		printf(CS("Touch speed: %.3f"), dist / (float)(touchUp.time - touchDown.time));
#endif
		opSendTouch(ctx, touch, TOUCH_DOWN);

	}else if (total && !ctx->pressed){
		ctx->pressed = 1;
		touchCtx.touchDragTotal = 0;
		//printf(CS("\nPressed"));
		touch->x = touch->points[0].x;
		touch->y = touch->points[0].y;

		touchDown = *touch;
		touchUp = *touch;
		
		//printf(CS("\nTouch down: %i, %i"), touchDown.x, touchDown.y);
		opSendTouch(ctx, touch, TOUCH_UP);
	}
}


#endif
