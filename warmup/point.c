#include <assert.h>
#include "common.h"
#include "point.h"
#include <math.h>
//#include <stdlib.h>

void
point_translate(struct point *p, double x, double y)
{
	p->x += x;
	p->y += y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
	double deltaX = p1->x > p2->x ? p1->x - p2->x : p2->x - p1->x;
	double deltaY = p1->y > p2->y ? p1->y - p2->y : p2->y - p1->y;
	double distance = sqrt(deltaX * deltaX + deltaY * deltaY);
	return distance;
}

int point_compare(const struct point *p1, const struct point *p2){
    struct point* origin = malloc(sizeof(*origin));
    origin = point_set(origin,0,0);
	double dis1 = point_distance(p1,origin);
    double dis2 = point_distance(p2,origin);
    if(dis1 == dis2)
        return 0;
    if(dis1 > dis2)    
        return 1;
    else
        return -1;
}
