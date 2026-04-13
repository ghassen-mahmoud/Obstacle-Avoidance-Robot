#include <msp430.h>
#include <ADC.h>

#define PWM_PERIOD 75

#define FRONT_THRESHOLD 800
#define LEFT_THRESHOLD 350
#define LUX_THRESHOLD 950

#define TURN_90 335

#define EXIT_EXTRA_MS 500
#define LEFT_CONFIRM_MS 680
#define STOP_DELAY 5
#define LOOP_DELAY 5

#define LEFT_CHANNEL 3
#define LUX_CHANNEL 4
#define FRONT_CHANNEL 1

#define LED_LEFT  BIT0
#define LED_FRONT BIT6

unsigned int left_value = 0;
unsigned int front_value = 0;
unsigned int lux_value = 0;

typedef enum {
    STATE_DECIDE = 0,
    STATE_LEFT,
    STATE_FORWARD,
    STATE_RIGHT,
    STATE_LUX,
} State;

State state = STATE_DECIDE;

void delay_ms(unsigned int ms) {
    while (ms--)
        __delay_cycles(1000);
}

void stop() {
    TA1CCR1 = 0;
    TA1CCR2 = 0;
}

void forward() {
    P2OUT |= BIT5;
    P2OUT &= ~BIT1;
    TA1CCR1 = PWM_PERIOD - 10;
    TA1CCR2 = PWM_PERIOD;
}

void forward_ms(unsigned int ms) {
    forward();
    delay_ms(ms);
    stop();
    delay_ms(STOP_DELAY);
}

void turnRight_inplace() {
    P2OUT &= ~BIT5;
    P2OUT &= ~BIT1;
    TA1CCR1 = 100;
    TA1CCR2 = 100;
}

void turnRight() {
    turnRight_inplace();
    delay_ms(TURN_90 + 8);
    stop();
    delay_ms(STOP_DELAY);
}

void turnLeft_inplace() {
    P2OUT |= BIT5;
    P2OUT |= BIT1;
    TA1CCR1 = 100;
    TA1CCR2 = 100;
}

void turnLeft() {
    turnLeft_inplace();
    delay_ms(TURN_90);
    stop();
    delay_ms(STOP_DELAY);
}

// point 2 : lecture unique, pas de moyenne
unsigned int read_adc_fast(unsigned int ch) {
    ADC_Demarrer_conversion(ch);
    return ADC_Lire_resultat();
}

// point 2 : moyenne uniquement pour les decisions critiques (STATE_DECIDE)
unsigned int read_adc_averaged(unsigned int ch) {
    unsigned int sum = 0;
    unsigned int i;
    for (i = 0; i < 3; i++) {
        ADC_Demarrer_conversion(ch);
        sum += ADC_Lire_resultat();
    }
    return sum / 3;
}

// lecture complete avec moyenne -> STATE_DECIDE uniquement
void read_sensors() {
    lux_value   = read_adc_averaged(LUX_CHANNEL);
    left_value  = read_adc_averaged(LEFT_CHANNEL);
    front_value = read_adc_averaged(FRONT_CHANNEL);
}

// point 2 + point 3 : lecture rapide (1 mesure) pour toutes les boucles internes
void read_sensors_fast() {
    lux_value   = read_adc_fast(LUX_CHANNEL);
    left_value  = read_adc_fast(LEFT_CHANNEL);
    front_value = read_adc_fast(FRONT_CHANNEL);
}

unsigned char left_is_open() {
    return left_value <= LEFT_THRESHOLD;
}

unsigned char front_is_open() {
    return front_value <= FRONT_THRESHOLD;
}

// point 6 : fonction centralisee
unsigned char wall_too_close() {
    return left_value > 900;
}

void clock_init() {
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL  = CALDCO_1MHZ;
}

void motor_init() {
    WDTCTL = WDTPW | WDTHOLD;

    P2DIR |= BIT1 | BIT5;
    P2DIR |= BIT2 | BIT4;
    P2SEL |= BIT2 | BIT4;
    P2SEL2 &= ~(BIT2 | BIT4);

    TA1CCR0  = 100;
    TA1CCTL1 = OUTMOD_7;
    TA1CCTL2 = OUTMOD_7;
    TA1CCR1  = 0;
    TA1CCR2  = 0;
    TA1CTL   = TASSEL_2 | MC_1;
}

void led_init() {
    P1OUT &= ~(LED_LEFT | LED_FRONT);
    P1DIR |=  (LED_LEFT | LED_FRONT);
}

int main() {

    clock_init();
    motor_init();
    led_init();
    ADC_init();

    read_sensors();      // moyenne au demarrage
    stop();
    delay_ms(STOP_DELAY);

    state = STATE_DECIDE;

    while (1) {
        switch (state) {

            case STATE_DECIDE: {
                read_sensors();              // moyenne : decision importante

                if (lux_value > LUX_THRESHOLD) {
                    state = STATE_LUX;
                    break;
                }

                if (left_is_open()) {
                    forward_ms(LEFT_CONFIRM_MS);
                    read_sensors();          // moyenne : confirmation de couloir

                    if (left_is_open()) {
                        state = STATE_LEFT;
                    } else if (front_is_open()) {
                        state = STATE_FORWARD;
                    } else {
                        state = STATE_RIGHT;
                    }
                } else if (front_is_open()) {
                    state = STATE_FORWARD;
                } else {
                    state = STATE_RIGHT;
                }
                break;
            }

            case STATE_LUX: {
                stop();
                delay_ms(STOP_DELAY * 30);
                while (1) {
                    read_sensors_fast();     // point 2+3 : rapide, pas besoin de precision
                    if (lux_value < LUX_THRESHOLD) {
                        state = STATE_DECIDE;
                        break;
                    }
                    delay_ms(LOOP_DELAY);
                }
                break;
            }

            case STATE_LEFT: {
                stop();
                delay_ms(STOP_DELAY);
                turnLeft();

                while (1) {
                    read_sensors_fast();     // point 2+3 : reaction rapide

                    if (wall_too_close()) {  // point 6
                        turnRight_inplace();
                        delay_ms(8);
                        state = STATE_DECIDE;
                        break;
                    }

                    if (left_is_open()) {
                        forward_ms(EXIT_EXTRA_MS);
                        state = STATE_DECIDE;
                        break;
                    }

                    if (!front_is_open()) {
                        stop();
                        delay_ms(STOP_DELAY);
                        state = STATE_DECIDE;
                        break;
                    }

                    forward();
                    delay_ms(LOOP_DELAY);
                }
                break;
            }

            case STATE_FORWARD: {
                while (1) {
                    read_sensors_fast();

                    if (wall_too_close()) {
                        turnRight_inplace();
                        delay_ms(8);
                        state = STATE_DECIDE;
                        break;
                    }

                    if (!front_is_open()) {   // obstacle détecté
                        stop();
                        delay_ms(STOP_DELAY);
                        state = STATE_DECIDE;
                        break;
                    }

                    if (left_is_open()) {     // couloir à gauche détecté
                        state = STATE_DECIDE;
                        break;
                    }

                    forward();                // avance EN CONTINU
                    delay_ms(LOOP_DELAY);     // 5ms entre chaque lecture capteur
                }
                break;
            }

            case STATE_RIGHT: {

                turnRight();

                while (1) {
                    read_sensors_fast();     // point 2+3 : reaction rapide
                    if (left_is_open()) {
                                          forward_ms(EXIT_EXTRA_MS);
                                          state = STATE_DECIDE;
                                          break;
                    }
                    if (wall_too_close()) {  // point 6
                        turnRight_inplace();
                        delay_ms(8);
                        state = STATE_DECIDE;
                        break;
                    }




                    if (!front_is_open()) {
                        stop();
                        delay_ms(STOP_DELAY);
                        state = STATE_DECIDE;
                        break;
                    }

                    forward();
                    delay_ms(LOOP_DELAY);
                }
                break;
            }

            default: {
                state = STATE_DECIDE;
                break;
            }
        }
    }
}
