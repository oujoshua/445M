long PID_Compute(unsigned long input, int id);

// set the Kp, Kd, and Ki terms for the user specified by id
void PID_SetConstants(int p, int d, int i, int id);

// set the target [input] for the user specified by id
void PID_SetTarget(int t, int id);

void PID_Disable(int id);

void PID_Enable(int id);
