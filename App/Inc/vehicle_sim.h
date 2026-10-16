#ifndef APP_INC_VEHICLE_SIM_H_
#define APP_INC_VEHICLE_SIM_H_

#include "traffic_ui.h"

void vehicle_sim_init(void);
void vehicle_sim_step(traffic_lamp_t ns_lamp, traffic_lamp_t ew_lamp);

#endif /* APP_INC_VEHICLE_SIM_H_ */
