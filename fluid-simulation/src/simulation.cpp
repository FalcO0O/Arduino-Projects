#include <cmath>
#include <vector>
#include <Arduino.h>

// Ustawienia symulacji
const int WIDTH = 128;
const int HEIGHT = 64;
const int NUM_PARTICLES = 30;                 // Liczba cząstek
const int PARTICLE_RADIUS = 3;                // Promień wizualny cząstki
const float MAX_SPEED = 3.0f;                 // Maksymalna prędkość
const float FRICTION = 0.90f;                 // Współczynnik tarcia (damping)
const float GRAVITY_FORCE = 1.5f;             // Siła grawitacji
static float GRAVITY_DIRECTION = 0.5f * M_PI; // pi/2 => w dół

// Parametry lepkości (do "wygładzania" prędkości między cząstkami)
const float VISCOSITY = 0.05f;              // współczynnik lepkości (dostosuj eksperymentalnie)
const float VISCOSITY_RADIUS_FACTOR = 1.5f; // ile razy promień interakcji jest większy od średnicy cząstki?

class Particle
{
public:
    float x, y;   // położenie
    float vx, vy; // prędkość

    Particle(float _x, float _y)
        : x(_x), y(_y)
    {
        // Losowa prędkość początkowa w zakresie [-2, 2]
        vx = ((float)rand() / RAND_MAX) * 4.0f - 2.0f;
        vy = ((float)rand() / RAND_MAX) * 4.0f - 2.0f;
    }

    // Nakładanie sił zewnętrznych (np. grawitacja)
    void applyForces()
    {
        vx += GRAVITY_FORCE * cos(GRAVITY_DIRECTION);
        vy += GRAVITY_FORCE * sin(GRAVITY_DIRECTION);
    }

    // Aktualizacja pozycji na podstawie prędkości
    void updatePosition()
    {
        x += vx;
        y += vy;
    }

    // Ograniczenie prędkości i zastosowanie tarcia
    void applyFrictionAndLimitSpeed()
    {
        // Ograniczenie prędkości
        float speed = sqrt(vx * vx + vy * vy);
        if (speed > MAX_SPEED)
        {
            float scale = MAX_SPEED / speed;
            vx *= scale;
            vy *= scale;
        }
        // Tarcie (damping)
        vx *= FRICTION;
        vy *= FRICTION;

        // Ustalanie progu minimalnej prędkości
        if (fabs(vx) < 0.1f)
            vx = 0;
        if (fabs(vy) < 0.1f)
            vy = 0;
    }

    // Obsługa kolizji ze ścianami
    void handleWallCollision()
    {
        // Odbicie od ścian w osi X
        if (x < PARTICLE_RADIUS)
        {
            x = PARTICLE_RADIUS;
            vx = -vx * 0.8f;
        }
        else if (x > WIDTH - PARTICLE_RADIUS)
        {
            x = WIDTH - PARTICLE_RADIUS;
            vx = -vx * 0.8f;
        }

        // Odbicie od ścian w osi Y
        if (y < PARTICLE_RADIUS)
        {
            y = PARTICLE_RADIUS;
            vy = -vy * 0.8f;
        }
        else if (y > HEIGHT - PARTICLE_RADIUS)
        {
            y = HEIGHT - PARTICLE_RADIUS;
            vy = -vy * 0.5f;
        }
    }
};

class Simulation
{
public:
    std::vector<Particle> particles;

    void init()
    {
        // Inicjalizacja cząstek
        for (int i = 0; i < NUM_PARTICLES; ++i)
        {
            float px = PARTICLE_RADIUS + ((float)rand() / RAND_MAX) * (WIDTH - 2 * PARTICLE_RADIUS);
            float py = PARTICLE_RADIUS + ((float)rand() / RAND_MAX) * (HEIGHT - 2 * PARTICLE_RADIUS);
            particles.emplace_back(px, py);
        }
    }

    void setGravityDirection(float direction)
    {
        GRAVITY_DIRECTION = direction + M_PI / 2;
    }

    // Krok: "lepkość" - zbliżanie prędkości sąsiadujących cząstek
    void applyViscosity()
    {
        // Promień interakcji: np. 2 (średnica cząstki) * factor
        float interactionRadius = (2.0f * PARTICLE_RADIUS) * VISCOSITY_RADIUS_FACTOR;
        float interactionRadiusSq = interactionRadius * interactionRadius;

        for (size_t i = 0; i < particles.size(); ++i)
        {
            for (size_t j = i + 1; j < particles.size(); ++j)
            {
                Particle &p1 = particles[i];
                Particle &p2 = particles[j];

                float dx = p2.x - p1.x;
                float dy = p2.y - p1.y;
                float distSq = dx * dx + dy * dy;

                if (distSq < interactionRadiusSq && distSq > 0.0001f)
                {
                    // Różnica prędkości
                    float dvx = p2.vx - p1.vx;
                    float dvy = p2.vy - p1.vy;

                    // Proste "wygładzanie" - przesuwamy cząstki w stronę średniej prędkości
                    // Im większy VISCOSITY, tym bardziej ujednolicamy prędkości
                    float factor = VISCOSITY;

                    p1.vx += factor * dvx;
                    p1.vy += factor * dvy;
                    p2.vx -= factor * dvx;
                    p2.vy -= factor * dvy;
                }
            }
        }
    }

    void nextFrame()
    {
        // 1. Nakładamy siły zewnętrzne (np. grawitacja)
        for (auto &p : particles)
        {
            p.applyForces();
        }

        // 2. Symulujemy lepkość (redukujemy różnice prędkości cząstek, które są blisko)
        applyViscosity();

        // 3. Zaktualizuj pozycje cząstek
        for (auto &p : particles)
        {
            p.updatePosition();
        }

        // 4. Iteracyjne rozwiązywanie kolizji między cząstkami
        const int COLLISION_ITERATIONS = 2; // liczba iteracji
        for (int iter = 0; iter < COLLISION_ITERATIONS; ++iter)
        {
            for (size_t i = 0; i < particles.size(); ++i)
            {
                for (size_t j = i + 1; j < particles.size(); ++j)
                {
                    Particle &p1 = particles[i];
                    Particle &p2 = particles[j];

                    float dx = p2.x - p1.x;
                    float dy = p2.y - p1.y;
                    float distSq = dx * dx + dy * dy;
                    float dist = sqrt(distSq);
                    float minDist = 2.0f * PARTICLE_RADIUS;

                    // Sprawdź czy cząstki nachodzą na siebie
                    if (dist < minDist && dist > 0)
                    {
                        // Oblicz nachodzenie
                        float overlap = minDist - dist;
                        // Wyznacz wektor normalny
                        float nx = dx / dist;
                        float ny = dy / dist;

                        // Korekta pozycji – rozdziel cząstki po równo
                        p1.x -= (overlap / 2.0f) * nx;
                        p1.y -= (overlap / 2.0f) * ny;
                        p2.x += (overlap / 2.0f) * nx;
                        p2.y += (overlap / 2.0f) * ny;

                        // Proste zderzenie sprężyste (dla cząstek o równej masie):
                        float rvx = p2.vx - p1.vx;
                        float rvy = p2.vy - p1.vy;
                        float velAlongNormal = rvx * nx + rvy * ny;
                        if (velAlongNormal < 0)
                        {
                            float restitution = 0.8f; // współczynnik odbicia
                            float impulse = -(1.0f + restitution) * velAlongNormal;
                            impulse /= 2.0f; // dzielimy przez sumę mas (tutaj m1=m2=1)
                            float impX = impulse * nx;
                            float impY = impulse * ny;
                            p1.vx -= impX;
                            p1.vy -= impY;
                            p2.vx += impX;
                            p2.vy += impY;
                        }
                    }
                }
            }
        }

        // 5. Obsługa kolizji z krawędziami i nałożenie tarcia
        for (auto &p : particles)
        {
            p.handleWallCollision();
            p.applyFrictionAndLimitSpeed();
        }
    }
};
