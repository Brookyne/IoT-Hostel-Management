
// #include <M5Stack.h>
// #include <M5GFX.h>
// #include "UNIT_ACMEASURE.h"

// M5GFX display;
// M5Canvas canvas(&display);
// UNIT_ACMEASURE sensor;

// void setup() {
//     M5.begin(true, false, true);
//     M5.Power.begin();
//     display.begin();
//     canvas.setColorDepth(8);  // mono color
//     canvas.setFont(&fonts::efontCN_12);
//     canvas.createSprite(display.width(), display.height());
//     canvas.setTextSize(2);
//     while (!(sensor.begin(&Wire, UNIT_ACMEASURE_ADDR, 21, 22, 100000UL))) {
//         canvas.clear(BLACK);
//         canvas.setCursor(10, 0);
//         canvas.println("No Module!");
//         canvas.pushSprite(0, 0);
//     }
// }

// void loop() {
//     canvas.clear(BLACK);
//     canvas.setTextSize(2);
//     canvas.setTextColor(ORANGE);
//     canvas.drawString("Unit ACMeasure String", 10, 0);
//     canvas.setColor(ORANGE);
//     canvas.drawFastHLine(10, 30, 300);
//     if (sensor.getReady()) {
//         canvas.setTextColor(YELLOW);
//         canvas.setTextSize(2);
//         canvas.setCursor(10, 40);
//         canvas.printf("Voltage:");
//         canvas.setTextSize(3);
//         canvas.printf("%.2fV", (float)(sensor.getVoltage()) / 100.0);
//         canvas.setTextColor(GREEN);
//         canvas.setCursor(10, 80);
//         canvas.setTextSize(2);
//         canvas.printf("Current:");
//         canvas.setTextSize(3);
//         canvas.printf("%.2fA", (float)(sensor.getCurrent()) / 100.0);
//         canvas.setTextSize(2);
//         canvas.setTextColor(YELLOW);
//         canvas.setCursor(10, 120);
//         canvas.setTextSize(2);
//         canvas.printf("Power:");
//         canvas.setTextSize(3);
//         canvas.printf("%.2fW", (float)(sensor.getPower()) / 100.0);
//         canvas.setTextSize(2);
//         canvas.setTextColor(GREEN);
//         canvas.setCursor(10, 160);
//         canvas.setTextSize(2);
//         canvas.printf("Power Factor:");
//         canvas.setTextSize(3);
//         canvas.printf("%.2f", (float)(sensor.getPowerFactor()) / 100.0);
//         canvas.setTextSize(2);
//     } else {
//         canvas.setTextColor(RED);
//         canvas.drawString("Data not good", 10, 40);
//     }
//     canvas.pushSprite(0, 0);
// }
