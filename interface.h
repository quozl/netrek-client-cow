/* interface.c */
void set_speed(int speed);
void set_course(unsigned char dir);
void shield_up(void);
void shield_down(void);
void shield_tog(void);
void bomb_planet(void);
void beam_up(void);
void beam_down(void);
void repair(void);
void repair_off(void);
void repeat_message(void);
void cloak(void);
void cloak_on(void);
void cloak_off(void);
unsigned long mstime(void);
unsigned long msetime(void);
void run_clock(time_t curtime);
