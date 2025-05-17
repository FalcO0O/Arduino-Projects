#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "simulation.cpp"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define POT_PIN 2
#define I2C_SCL 4
#define I2C_SDA 15
#define SCREEN_ADDRESS 0x3C
#define SIMULATION_SPEED 0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Simulation simulation;

void printFluid(std::vector<Particle>);
void initWire();
void initDisplay();

void setup()
{
	Serial.begin(38400);
	pinMode(POT_PIN, INPUT_PULLUP);
	initWire();
	initDisplay();
	simulation.init();

	display.clearDisplay(); // Clear the buffer
}

void loop()
{
	float gravDirection = (float)map(analogRead(POT_PIN), 0, 4095, 0, 360);
	simulation.setGravityDirection(gravDirection * (M_PI / 180.0f));
	simulation.nextFrame();
	printFluid(simulation.particles);
	delay(SIMULATION_SPEED);
}

void printFluid(std::vector<Particle> particles)
{
	display.clearDisplay();
	for (auto &p : particles)
	{
		display.fillCircle(p.x, p.y, PARTICLE_RADIUS + 1, SSD1306_WHITE);
	}
	display.display();
}

void initWire()
{
	if (!Wire.begin(I2C_SDA, I2C_SCL))
	{
		Serial.println(F("Wire initialization failed"));
		for (;;)
			; // Don't proceed, loop forever
	}
}

void initDisplay()
{
	if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
	{
		Serial.println(F("SSD1306 allocation failed"));
		for (;;)
			; // Don't proceed, loop forever
	}
}