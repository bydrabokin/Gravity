#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define G 6.6743e-11
#define PI 3.141592653589793

#define SCALE 1e5 // 1 px = 100,000m
#define SCREEN_CENTER_X 900
#define SCREEN_CENTER_Y 450

// structs
typedef struct {

        double x, y, vx, vy, v, a;
        double radius;
        double mass;
        double altitude;
        double vper;
        double drawx, drawy;

    } State;

typedef struct  {
    SDL_Texture *texture;
    int width, height;
    char cached_text[128]; 

} Textcache;

typedef struct {

    Textcache argument1, argument2;
    SDL_Rect rect1, rect2;
    SDL_Color colorRect1, colorRect2;
    int extraRoom1, extraRoom2;

} Toggle;

typedef struct {
        SDL_Rect gray;
        int ballx, bally;
        double radius;
        SDL_Rect blue;
        char name[128];
        double value;
        Textcache id;
        double min, max; 
        int textRectx, textRecty;
        double *affects; // affects = moon
        
        

} slider;

typedef struct {
    SDL_Rect buttonRect;
    SDL_Color colorRect1, colorBorders;
    Textcache text;
    int textRectx, textRecty;

} Button;


State moon, earth;

double gforce, r, a, a2, dx, dy, dvx, dvy, ax, ay;
double DT, zoomFactor;
double separation;
int counter;
int startTransfer;

// Colors
SDL_Color white;
SDL_Color gray1;
SDL_Color blue1, blue2, blue3, blue4, blue5;
SDL_Color orange1, orange2;
SDL_Color green1, green2;
SDL_Color red, red1, red2;
SDL_Color purple1, purple2;
SDL_Color cyan1, cyan2;

 // points
SDL_Point moonTrail[100];
SDL_Point earthTrail[100];

// buttons
Button orbitalSpeedButton;
Button orbitalSpeedButton2;
Button LEObut, MEObut, GEObut, HEObut, COMbut, parabolicButton;
Button trailsBut;

// sliders
slider massslider1;
slider massslider2;
slider radiusslider1;
slider radiusslider2;
slider altitudeSlider;
slider velocitySlider, velocitySlider1, velocitySlider2;
slider initialSeparationSlider;
slider dtSlider;
slider zoomSlider;

Textcache warning;

SDL_Event e; // pointer to events

// Derivative
void derivative(State s, State *d, State s2, State *d2) {
    dx = s.x -s2.x;
    dy = s.y -s2.y;

    r = sqrt(dx*dx+dy*dy);
    if (r < 50.0) {r = 50.0;}

    a = (G * s2.mass) / (r*r);

    d->x = s.vx;
    d->y = s.vy;

    d->vx = -a * (dx/r);
    d->vy = -a * (dy/r);


    a2 = (G * s.mass) / (r*r);

    d2->x = s2.vx;
    d2->y = s2.vy;

    d2->vx = a2 * (dx/r);
    d2->vy = a2 * (dy/r);

    
}

void drawCircle(SDL_Renderer *renderer,  double cx, double cy, int radius, SDL_Color color) {

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

    for (int y = -radius; y <= radius; y++) {
        int dx = (int)sqrt(radius*radius - y*y);
        SDL_RenderDrawLine(renderer, (int)cx - dx, (int)cy + y, (int) cx + dx, (int) cy + y);
    }

}

void setOrbitalSpeed(State *planet, State *sat, int mode) {

    double totalMass, R_Moon, R_Earth, separation, com, w;
    totalMass = sat->mass + planet->mass; 
    com = (planet->mass * planet->x + sat->mass * sat->x) / totalMass;
    separation = sat->x - planet->x;


    // force of gravity must equal to centripetal force
    // Fce = mw²r
    // Fg = G * (m1*m2) / r² 
    // w = sqrt(gforce/(m*r))
    // gforce = G * (moon.mass * earth.mass) / separation*separation;
    // w = sqrt((G * totalMass) / (separation * separation * separation));
    // then you just convert the w which is in rad/s to ms by multiplying by R

    w = sqrt((G * totalMass) / (separation * separation * separation));

    sat->vy = w * (sat->x - com);
    planet->vy = w * (planet->x - com);

    if (mode == 2) {
        sat->vy *= moon.vper;
        planet->vy *= earth.vper;
    }

} 

void reset(State *a, State *b, slider *massslider, slider *radius1, slider *radius2, int mode, double alt, bool hardreset) {
    
    char buffer[256];

    if (mode == 1){
        a->x = SCREEN_CENTER_X * SCALE;
        a->y = SCREEN_CENTER_Y * SCALE;
        a->vx = 0;
        a->vy = 0;
        
        b->y = SCREEN_CENTER_Y * SCALE;
        b->vx = 0;
        b->vy = 0;
        
        
        b->altitude = alt;
        b->x = a->x + a->radius + b->radius + b->altitude;
        

        massslider->min = 5e21;
        massslider->max = 5e23;

        if (hardreset == true) {
            a->radius = 63.78 * SCALE;  
            a->mass = 5e25;
            b->radius = 17.374 * SCALE;
            b->mass = 5e22;


        }

    } else if (mode == 2) {
        a->x =  SCREEN_CENTER_X * SCALE - separation / 2 ;
        a->y = SCREEN_CENTER_Y * SCALE;
        a->vx = 0;
        a->vy = 0;


        b->x = SCREEN_CENTER_X * SCALE + separation / 2;
        b->y = SCREEN_CENTER_Y * SCALE;
        b->vx = 0;
        b->vy = 0;
        b->altitude = 0;  

        if (hardreset == true) {
            a->radius = 40 * SCALE;
            b->radius = 40 * SCALE;
            b->mass = 5e25;
            a->mass = 5e25;
            a->vper = 1;
            b->vper = 1;
            a->x = 650 * SCALE;
            b->x = 1150 * SCALE;
            
            
        }

        massslider->min = 5e24;
        massslider->max = 5e26;

        
    }

    if (mode == 2 && hardreset == false) {
        a->x = 650  * SCALE;
        b->x = 1150 * SCALE;
        setOrbitalSpeed(a,  b, mode);
        a->x = 900 * SCALE - separation / 2;
        b->x = 900 * SCALE + separation / 2;
    } else {
        setOrbitalSpeed(a,  b, mode);
    }

    if (hardreset == true) {
        DT = 15000.0 / 55.0;
        zoomFactor = 1e5;
    }

    memset(moonTrail, 0, sizeof moonTrail);
    memset(earthTrail, 0, sizeof earthTrail);
    
    startTransfer = 0;

    counter = 0;
}

void updateCachedText(Textcache *cache, TTF_Font* font, const char *text, SDL_Renderer *renderer) {

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};
    SDL_Color color;

    if (cache == &warning) {
        color = red;
    } else {
        color = white;
    }

    if (strcmp(cache->cached_text, text) == 0) {
        return;
    }

    if (cache->texture) {
        SDL_DestroyTexture(cache->texture);
    }

    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    cache->texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_QueryTexture(cache->texture, NULL, NULL, &cache->width, &cache->height);

    strcpy(cache->cached_text, text);
    SDL_FreeSurface(surface);
    
}

void render_text(Textcache *cache, SDL_Renderer *renderer, int x, int y) {
    SDL_Rect textRect = {x, y, cache->width, cache->height};
    SDL_RenderCopy(renderer, cache->texture, NULL, &textRect);
}

void drawToggle(Toggle *toggle, SDL_Renderer *renderer, TTF_Font* font, int mode) {
    
        
        SDL_SetRenderDrawColor(renderer, toggle->colorRect2.r, toggle->colorRect2.g, toggle->colorRect2.b, toggle->colorRect2.a);
        SDL_RenderFillRect(renderer, &toggle->rect2);

        SDL_SetRenderDrawColor(renderer, toggle->colorRect1.r, toggle->colorRect1.g, toggle->colorRect1.b, toggle->colorRect1.a);
        SDL_RenderFillRect(renderer, &toggle->rect1);


        render_text(&toggle->argument1, renderer, toggle->rect1.x + toggle->extraRoom1, toggle->rect1.y + 7);
        render_text(&toggle->argument2, renderer, toggle->rect2.x + toggle->extraRoom2, toggle->rect2.y + 7);
}

void drawSlider(SDL_Renderer *renderer, slider theslider, TTF_Font* font) {
    
    
    SDL_Color white = {255, 255, 255, 255};

    SDL_Rect textRect;
    

    render_text(&theslider.id, renderer, theslider.textRectx, theslider.textRecty);
    

    SDL_SetRenderDrawColor(renderer, gray1.r, gray1.g, gray1.b, 255);
    SDL_RenderFillRect(renderer, &theslider.gray);

    SDL_SetRenderDrawColor(renderer, blue1.r, blue1.g, blue1.b, 255);
    SDL_RenderFillRect(renderer, &theslider.blue);

    drawCircle(renderer, theslider.ballx, theslider.bally, theslider.radius, white);

}

void getInputSlider(slider *theslider) {

    int mouseX, mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX, &mouseY);
    double normalized, distance;
    distance = sqrt((mouseX-theslider->ballx)*(mouseX-theslider->ballx)+(mouseY-theslider->bally)*(mouseY-theslider->bally));

    if (distance < 20.0) {
        
                
        theslider->ballx = mouseX;
        

        if (theslider->ballx < theslider->gray.x) theslider->ballx = theslider->gray.x;
        if (theslider->ballx > theslider->gray.x + theslider->gray.w) theslider->ballx = theslider->gray.x + theslider->gray.w;
        
        theslider->blue = (SDL_Rect){theslider->gray.x, theslider->gray.y, theslider->ballx -theslider->gray.x, 15};
        
        normalized = (mouseX - theslider->gray.x) / (double)theslider->gray.w;
        
        
        if (theslider == &massslider1 || theslider == &massslider2 || theslider == &dtSlider || theslider == &zoomSlider) {  
            *theslider->affects = theslider->min * pow(theslider->max / theslider->min, normalized);
        } else {
            *theslider->affects = theslider->min + normalized * (theslider->max - theslider->min);
        }
        
        if (*theslider->affects > theslider->max) {*theslider->affects = theslider->max;}
        if (*theslider->affects < theslider->min) {*theslider->affects = theslider->min;} 

    }
}

bool getInputButton(Button *thebutton) {
    int mouseX, mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX, &mouseY);
    
    if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        if (mouseX > thebutton->buttonRect.x &&
            mouseX < thebutton->buttonRect.x + thebutton->buttonRect.w &&
            mouseY > thebutton->buttonRect.y &&
            mouseY < thebutton->buttonRect.y + thebutton->buttonRect.h) {
            return true;
        }
    }
    return false;
}

bool isItApoapsis() {
    dx = moon.x - earth.x;
    dy = moon.y - earth.y;
    dvx = moon.vx - earth.vx;
    dvy = moon.vy - earth.vy;
    r = sqrt(dx*dx + dy*dy);
    double radial_velocity = (dx*dvx + dy*dvy) / r;
    if (radial_velocity <= 10) {
        return true;
    }

    return false;

}

int getInputToggle(Toggle *thetoggle) {
    int mouseX, mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX, &mouseY);
    
    if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        if (mouseX > thetoggle->rect1.x &&
            mouseX < thetoggle->rect1.x + thetoggle->rect1.w &&
            mouseY > thetoggle->rect1.y &&
            mouseY < thetoggle->rect1.y + thetoggle->rect1.h) {
            return 1;
        } else if (mouseX > thetoggle->rect2.x &&
            mouseX < thetoggle->rect2.x + thetoggle->rect1.w &&
            mouseY > thetoggle->rect2.y &&
            mouseY < thetoggle->rect2.y + thetoggle->rect2.h) {
            return 2;
        }
    }

    return 0;
}

void drawButton(SDL_Renderer *renderer, Button *but) {

    if (getInputButton(but)) {
        SDL_SetRenderDrawColor(renderer, but->colorBorders.r, but->colorBorders.g, but->colorBorders.b, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, but->colorRect1.r, but->colorRect1.g, but->colorRect1.b, 255);
    }
    
    SDL_RenderFillRect(renderer, &but->buttonRect);

    if (getInputButton(but)) {
        SDL_SetRenderDrawColor(renderer, but->colorRect1.r, but->colorRect1.g, but->colorRect1.b, 255);

    } else {
        SDL_SetRenderDrawColor(renderer, but->colorBorders.r, but->colorBorders.g, but->colorBorders.b, 255);
    }
    SDL_Rect hborder1 = {but->buttonRect.x, but->buttonRect.y, but->buttonRect.w, 5};
    SDL_Rect hborder2 = {but->buttonRect.x, but->buttonRect.y + but->buttonRect.h - 5, but->buttonRect.w, 5};
    SDL_Rect vborder1 = {but->buttonRect.x, but->buttonRect.y +5, 5, but->buttonRect.h -10};
    SDL_Rect vborder2 = {but->buttonRect.x + but->buttonRect.w - 5 , but->buttonRect.y+5, 5, but->buttonRect.h -10};
    
    SDL_RenderFillRect(renderer, &hborder1);
    SDL_RenderFillRect(renderer, &hborder2);
    SDL_RenderFillRect(renderer, &vborder1);
    SDL_RenderFillRect(renderer, &vborder2);

    render_text(&but->text, renderer, but->textRectx, but->textRecty);


}

void setOrbitalSpeedOneBody() {
    moon.v = sqrt((G * earth.mass) / r);
    moon.vx = -moon.v * (dy/r);
    moon.vy = -moon.v * (dx/r);
}

void renderImageRotated(SDL_Renderer *renderer, SDL_Texture *texture, int x, int y, int w, int h, double angle) {
    SDL_Rect dest = {x, y, w, h};
    
    SDL_Point center = {w / 2, 0};  
    
    SDL_RenderCopyEx(renderer, texture, NULL, &dest, angle, &center, SDL_FLIP_NONE);
}

int main() {

    

    // Colors
    gray1   = (SDL_Color){67, 67, 67, 255};
    blue1   = (SDL_Color){60, 120, 180, 255};
    blue2   = (SDL_Color){49, 112, 143, 255};
    blue3   = (SDL_Color){36, 82, 105, 255};
    orange1 = (SDL_Color){255,141,0, 255};
    orange2 = (SDL_Color){255,116,0, 255};
    blue4   = (SDL_Color){9, 0, 136, 255};
    blue5   = (SDL_Color){2, 0, 108, 255};
    green1  = (SDL_Color){48, 104, 68, 255};
    green2  = (SDL_Color){44, 76, 59, 255};
    red     = (SDL_Color){255, 0, 0, 255};
    red1    = (SDL_Color){178, 34, 34, 255};
    red2    = (SDL_Color){139, 0, 0, 255};
    white   = (SDL_Color){255, 255, 255, 255};
    purple1 = (SDL_Color){128, 0, 128, 255};
    purple2 = (SDL_Color){102, 0, 102, 255};
    cyan1   = (SDL_Color){0,139,139, 255};
    cyan2   = (SDL_Color){0, 100, 100, 255};
    
    
    
    reset(&earth, &moon, &massslider2, &radiusslider1, &radiusslider2, 1, 250.0 * SCALE, true);
    
    moon.x = moon.altitude + earth.x + earth.radius;

    // ORBITAL SPEED
    setOrbitalSpeed(&earth, &moon, 1);
    earth.v = earth.vy;
    moon.v = moon.vy;


    // distances
    dx = moon.x -earth.x;
    dy = moon.y -earth.y;
    r = sqrt(dx*dx+dy*dy);

    
    // time things
    counter = 0;
    int lastTime = 0;
    int burningTime = 0;
    
    // triggers
    bool orbiting = false;
    bool running = true;
    bool drawing = true;
    bool escapeVelocityTrigger = true;
    bool play = false;
    bool set = false;
    bool COM = true;
    bool savingPoints = true;
    bool escaped = false;
    bool now = true;
    bool burning = false;

    // orbital
    double v, eccentricty, orbitalEnergy, angularMomentum, mu, semiMajorAxis, semiMinorAxis, periapsisAlt, apoapsisAlt, orbitalPeriod;
    double dv1, dv2, dv1_x, dv1_y, dv2_x, dv2_y, savedR;
    double totalMass, comx, comy;
    

    // trails
    int trailLengthMoon = 0, trailLengthEarth = 0;
    double lastPointSepartion;

    // extra
    int mousex, mousey;
    char buffer[256];
    
    // initial
    int mode = 1;
    int option = 1;
    separation = r;
    zoomFactor = SCALE;

    // transfers
    double targetR;
    double escapeVel = 0;
    double trigger, difference;
    double tempAlt = moon.altitude;

    // k4
    State k1_moon, k2_moon, k3_moon, k4_moon, tempMoon;
    State k1_earth, k2_earth, k3_earth, k4_earth, tempEarth;


    // audio
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO); // init sdl2
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    // window
    SDL_Window *win = SDL_CreateWindow("Graviity", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1800, 900, 0); // Init window
    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // render

    // visual structures
    SDL_Rect signal = {1730, 830, 50, 50};

    // Font
    TTF_Init();
    TTF_Font* font   = TTF_OpenFont("resources/font/Monocraft.ttf", 18);
    TTF_Font* font2  = TTF_OpenFont("resources/font/Monocraft.ttf", 20);
    TTF_Font* font3  = TTF_OpenFont("resources/font/Monocraft.ttf", 13);
    TTF_Font* font4  = TTF_OpenFont("resources/font/Monocraft.ttf", 22);
    TTF_Font* specialFont  = TTF_OpenFont("resources/font/STIXTwoMath-Regular.otf", 22);

    // images
    SDL_Surface* raptorOFFimage = IMG_Load("resources/images/raptorOFF.png");
    SDL_Surface* raptorONimage = IMG_Load("resources/images/raptorON.png");
    
    SDL_Texture *raptorOFFsurface = SDL_CreateTextureFromSurface(renderer, raptorOFFimage);
    SDL_Texture *raptorONsurface = SDL_CreateTextureFromSurface(renderer, raptorONimage);

    SDL_FreeSurface(raptorOFFimage);
    SDL_FreeSurface(raptorONimage);


    // Text 
    char earthxyBuffer[256], earthvxvyaBuffer[256], moonxyBuffer[256], moonvxvyaBuffer[256], timePassedBuffer[256], orbitClassificationBuffer[256], eccentrictyBuffer[256], semiMajorAxisBuffer[256], semiMinorAxisBuffer[256], periapsisBuffer[256], apoapsisBuffer[256], muBuffer[256], orbitalEnergyBuffer[256], angularMomentumBuffer[256], orbitalPeriodBuffer[256], escapeVelBuffer[256], heomessageBuffer[256]; 
    Textcache earthxy = {0}, earthvxvya = {0}, moonxy = {0}, moonvxvya = {0}, timePassed = {0}, orbitClassSentence = {0}, orbitClassification = {0}, eccentrictyText = {0}, semiMajorAxisText = {0}, semiMinorAxisText = {0}, periapssisText = {0}, apoapsisText = {0}, muText = {0}, orbitalEnergyText = {0}, angularMomentumText = {0}, orbitalPeriodText = {0}, escapeVelText = {0}, heomessage = {0}, sorryText = {0}, modeText = {0}, bodies = {0}, orbits = {0}, hohmannText = {0}, pressS = {0}, pressR = {0}, pressH = {0}, realTime = {0}, orbitalParameters = {0};

    updateCachedText(&orbitClassSentence, font4, "Orbital Clasification:", renderer);
    updateCachedText(&sorryText, font3, "This mechanic might be a bit funky :/", renderer);
    updateCachedText(&modeText, font2, "Mode:", renderer);
    updateCachedText(&bodies, font2, "Planet       | Satellite", renderer);
    warning = (Textcache){0};
    updateCachedText(&warning, font2, "*Warning", renderer);
    updateCachedText(&orbits, font4, "Apply Precomputed Orbits:", renderer);
    updateCachedText(&hohmannText, font4, "Transfer to: ", renderer);
    updateCachedText(&pressR, font4, "*Press R to reset.", renderer);
    updateCachedText(&pressH, font4, "*Press H to hide UI.", renderer);
    updateCachedText(&pressS, font4, "*Press SPACE to start/stop simulation.", renderer);
    updateCachedText(&realTime, font4, "Real-Time Parameters:", renderer);
    updateCachedText(&orbitalParameters, font4, "Orbital Parameters: ", renderer);
    
    
    // set trails to 0
    memset(moonTrail, 0, sizeof moonTrail);
    memset(earthTrail, 0, sizeof earthTrail);

    // Audio
    Mix_Chunk *escapeVelmp3       = Mix_LoadWAV("resources/sound/voice/escapeVelocity.mp3");
    Mix_Chunk *groundCollisionmp3 = Mix_LoadWAV("resources/sound/voice/groundCollsion.mp3");

    // Sliders
    // mode 1: min -> 1e17, max -> 1e22 min -> 1e14 1e19
    massslider1 = (slider){0};
    massslider1.gray = (SDL_Rect){20, 150, 150, 15};
    massslider1.min = 5e24;
    massslider1.max = 5e26;
    massslider1.ballx = 20 + (log(earth.mass) - log(massslider1.min)) / (log(massslider1.max) - log(massslider1.min)) * 150;
    massslider1.bally = 157;
    massslider1.radius = 15;
    massslider1.blue = (SDL_Rect){20, 150, massslider1.ballx-20, 15};
    massslider1.value = earth.mass;
    massslider1.textRectx = 20;
    massslider1.textRecty = 120;
    strcpy(massslider1.name, "Mass (kg)");
    snprintf(buffer, sizeof(buffer), "%.2e  %s", massslider1.value, massslider1.name);
    updateCachedText(&massslider1.id, font, buffer, renderer);
    massslider1.affects = &earth.mass;

    massslider2 = (slider){0};
    massslider2.gray = (SDL_Rect){190, 150, 150, 15};
    massslider2.min = 5e21;
    massslider2.max = 5e23;
    massslider2.ballx = 190 + (log(moon.mass) - log(massslider2.min)) / (log(massslider2.max) - log(massslider2.min)) * 150;
    massslider2.bally = 157;
    massslider2.radius = 15;
    massslider2.blue = (SDL_Rect){190, 150, massslider2.ballx -190, 15};
    massslider2.value = moon.mass;
    massslider2.textRectx = 270;
    massslider2.textRecty = 120;
    strcpy(massslider2.name, "");
    snprintf(buffer, sizeof(buffer), "%.2e   %s", massslider2.value, massslider2.name);
    updateCachedText(&massslider2.id, font, buffer, renderer);
    massslider2.affects = &moon.mass;

    // radius
    radiusslider1 = (slider){0};
    radiusslider1.gray = (SDL_Rect){20, 210, 150, 15};
    radiusslider1.min = 4 * SCALE;
    radiusslider1.max = 100 * SCALE;
    radiusslider1.ballx = 20 + (earth.radius - radiusslider1.min) / (radiusslider1.max - radiusslider1.min) * 150;
    radiusslider1.bally = 217;
    radiusslider1.radius = 15;
    radiusslider1.blue = (SDL_Rect){20, 210, radiusslider1.ballx-20, 15};
    radiusslider1.value = (int)(earth.radius / 1000);
    radiusslider1.textRectx = 20;
    radiusslider1.textRecty = 180;
    strcpy(radiusslider1.name, "Radius (km)");
    snprintf(buffer, sizeof(buffer), "%.0f  %s", radiusslider1.value, radiusslider1.name);
    updateCachedText(&radiusslider1.id, font, buffer, renderer);
    radiusslider1.affects = (double *)&earth.radius;

    radiusslider2 = (slider){0};
    radiusslider2.gray = (SDL_Rect){190, 210, 150, 15};
    radiusslider2.min = 4 * SCALE;
    radiusslider2.max = 100 * SCALE;
    radiusslider2.ballx = 190 + (moon.radius - radiusslider2.min) / (radiusslider2.max - radiusslider2.min) * 150;
    radiusslider2.bally = 217;
    radiusslider2.radius = 15;
    radiusslider2.blue = (SDL_Rect){190, 210, radiusslider2.ballx -190, 15};
    radiusslider2.value = (int)(moon.radius / 1000);
    radiusslider2.textRectx = 270;
    radiusslider2.textRecty = 180;
    strcpy(radiusslider2.name, "");
    snprintf(buffer, sizeof(buffer), "%.0f   %s", radiusslider2.value, radiusslider2.name);
    updateCachedText(&radiusslider2.id, font, buffer, renderer);
    radiusslider2.affects = (double *)&moon.radius;
    
    
    altitudeSlider = (slider){0};
    altitudeSlider.gray = (SDL_Rect){20, 310, 300, 15};
    altitudeSlider.min = 0;
    altitudeSlider.max = 1000 * SCALE;
    altitudeSlider.ballx = 20 + ((moon.x - earth.x) - altitudeSlider.min) / (altitudeSlider.max - altitudeSlider.min) * 300;
    altitudeSlider.bally = 317;
    altitudeSlider.radius = 15;
    altitudeSlider.blue = (SDL_Rect){20, 310, altitudeSlider.ballx -20, 15};
    altitudeSlider.value = (moon.altitude);
    altitudeSlider.textRectx = 40;
    altitudeSlider.textRecty = 280;
    strcpy(altitudeSlider.name, "Altitude (km)");
    snprintf(buffer, sizeof(buffer), "%s: %.0f", altitudeSlider.name, altitudeSlider.value);
    updateCachedText(&altitudeSlider.id, font, buffer, renderer);
    altitudeSlider.affects = &moon.altitude;

    velocitySlider = (slider){0};
    velocitySlider.gray = (SDL_Rect){20, 370, 300, 15};
    velocitySlider.min = 100;
    velocitySlider.max = 20000;
    velocitySlider.ballx = 20 + (fabs(moon.vy) - velocitySlider.min) / (velocitySlider.max - velocitySlider.min) * 300;
    velocitySlider.bally = 377;
    velocitySlider.radius = 15;
    velocitySlider.blue = (SDL_Rect){20, 370, velocitySlider.ballx -20, 15};
    velocitySlider.value = fabs(moon.vy);
    velocitySlider.textRectx = 30;
    velocitySlider.textRecty = 340;
    strcpy(velocitySlider.name, "Velocity (km/s)");
    snprintf(buffer, sizeof(buffer), "%s: %.2f", velocitySlider.name, velocitySlider.value);
    updateCachedText(&velocitySlider.id, font, buffer, renderer);
    velocitySlider.affects = &moon.v;

    
    velocitySlider1 = (slider){0};
    velocitySlider1.gray = (SDL_Rect){20, 380, 150, 15};
    velocitySlider1.min = 0;
    velocitySlider1.max = 1;
    velocitySlider1.ballx = 20 + (fabs(earth.vy) - velocitySlider1.min) / (velocitySlider1.max - velocitySlider1.min) * 150;
    velocitySlider1.bally = 387;
    velocitySlider1.radius = 15;
    velocitySlider1.blue = (SDL_Rect){20, 380, velocitySlider1.ballx -20, 15};
    velocitySlider1.value = earth.vper;
    velocitySlider1.textRectx = 20;
    velocitySlider1.textRecty = 350;
    strcpy(velocitySlider1.name, "    Velocity");
    snprintf(buffer, sizeof(buffer), "%.0f%% %s", velocitySlider1.value, velocitySlider1.name);
    updateCachedText(&velocitySlider1.id, font, buffer, renderer);
    velocitySlider1.affects = &earth.vper;

    velocitySlider2 = (slider){0};
    velocitySlider2.gray = (SDL_Rect){190, 380, 150, 15};
    velocitySlider2.min = 0;
    velocitySlider2.max = 1;
    velocitySlider2.ballx = 190 + (fabs(earth.vy) - velocitySlider2.min) / (velocitySlider2.max - velocitySlider2.min) * 150;
    velocitySlider2.bally = 387;
    velocitySlider2.radius = 15;
    velocitySlider2.blue = (SDL_Rect){190, 380, velocitySlider2.ballx - 190, 15};
    velocitySlider2.value = moon.vper;
    velocitySlider2.textRectx = 290;
    velocitySlider2.textRecty = 350;
    strcpy(velocitySlider2.name, "");
    snprintf(buffer, sizeof(buffer), "%.0f%% %s", velocitySlider2.value, velocitySlider2.name);
    updateCachedText(&velocitySlider2.id, font, buffer, renderer);
    velocitySlider2.affects = &moon.vper;

    initialSeparationSlider = (slider){0};
    initialSeparationSlider.gray = (SDL_Rect){20, 320, 300, 15};
    initialSeparationSlider.min = 200*SCALE;
    initialSeparationSlider.max = 2000 *SCALE;
    initialSeparationSlider.ballx = 20 + ((moon.x - earth.x) - initialSeparationSlider.min) / (initialSeparationSlider.max - initialSeparationSlider.min) * 300;
    initialSeparationSlider.bally = 327;
    initialSeparationSlider.radius = 15;
    initialSeparationSlider.blue = (SDL_Rect){20, 320, initialSeparationSlider.ballx - 20, 15};
    initialSeparationSlider.value = moon.x -earth.x;
    initialSeparationSlider.textRectx = 20;
    initialSeparationSlider.textRecty = 290;
    strcpy(initialSeparationSlider.name, "Separation (km)");
    snprintf(buffer, sizeof(buffer), "%s: %f", initialSeparationSlider.name, initialSeparationSlider.value);
    updateCachedText(&initialSeparationSlider.id, font, buffer, renderer);
    initialSeparationSlider.affects = &separation;

    dtSlider = (slider){0};
    dtSlider.gray = (SDL_Rect){20, 850, 300, 15};
    dtSlider.min = 10;
    dtSlider.max = 10000;
    dtSlider.ballx = 20 + (log(DT) - log(dtSlider.min)) / (log(dtSlider.max) - log(dtSlider.min)) * 300;
    dtSlider.bally = 857;
    dtSlider.radius = 15;
    dtSlider.blue = (SDL_Rect){20, 850, dtSlider.ballx - 20, 15};
    dtSlider.value = DT;
    dtSlider.textRectx = 20;
    dtSlider.textRecty = 822;
    strcpy(dtSlider.name, "Time warp");
    snprintf(buffer, sizeof(buffer), "%s: %.0fx", dtSlider.name, dtSlider.value);
    updateCachedText(&dtSlider.id, font, buffer, renderer);
    dtSlider.affects = &DT;

    zoomSlider = (slider){0};
    zoomSlider.gray = (SDL_Rect){1400, 870, 300, 15};
    zoomSlider.min = SCALE / 20;
    zoomSlider.max = SCALE * 15;
    zoomSlider.ballx = 1400 + (log(zoomFactor) - log(zoomSlider.min)) / (log(zoomSlider.max) - log(zoomSlider.min)) * 300;
    zoomSlider.bally = 877;
    zoomSlider.radius = 15;
    zoomSlider.blue = (SDL_Rect){1400, 850, zoomSlider.ballx - 1400, 15};
    zoomSlider.value = zoomFactor;
    zoomSlider.textRectx = 1400;
    zoomSlider.textRecty = 842;
    strcpy(zoomSlider.name, "Scale");
    snprintf(buffer, sizeof(buffer), "%s: 1px -> %.0fkm", zoomSlider.name, zoomSlider.value / 1000);
    updateCachedText(&zoomSlider.id, font, buffer, renderer);
    zoomSlider.affects = &zoomFactor;
    
    
    
    // density text
    Textcache densityT = {0};
    double densityEarth, densityMoon;
    densityEarth = earth.mass / (4.0/3.0 * PI * earth.radius * earth.radius * earth.radius * 1e15);
    densityMoon = moon.mass / (4.0/3.0 * PI * moon.radius * moon.radius * moon.radius * 1e15) ;
    snprintf(buffer, sizeof(buffer), "%.2f Density (kg/m3) %.2f", densityEarth, densityMoon);
    updateCachedText(&densityT, font, buffer, renderer);



    // Toggles
    Toggle modeToggle = {0};
    updateCachedText(&modeToggle.argument1, font, "1 body orbiting", renderer);
    updateCachedText(&modeToggle.argument2, font, "Binary System", renderer);
    modeToggle.rect1 = (SDL_Rect){80, 10, 220, 30};
    modeToggle.rect2 = (SDL_Rect){300, 10, 220, 30};
    modeToggle.colorRect1 = blue1;
    modeToggle.colorRect2 = gray1;
    modeToggle.extraRoom1 = 20;
    modeToggle.extraRoom2 = 35;

    Toggle orbitsToggle = {0};
    updateCachedText(&orbitsToggle.argument1, font, "Hohmann Transfers", renderer);
    updateCachedText(&orbitsToggle.argument2, font, "Precomputed Orbits", renderer);
    orbitsToggle.rect1 = (SDL_Rect){20, 490, 250, 30};
    orbitsToggle.rect2 = (SDL_Rect){270, 490, 250, 30};
    orbitsToggle.colorRect1 = gray1;
    orbitsToggle.colorRect2 = blue1;
    orbitsToggle.extraRoom1 = 25;
    orbitsToggle.extraRoom2 = 20;


    // Buttons
    orbitalSpeedButton = (Button){0};
    orbitalSpeedButton.buttonRect = (SDL_Rect){20, 415, 406, 50};
    orbitalSpeedButton.colorBorders = blue3;
    orbitalSpeedButton.colorRect1 = blue2;
    updateCachedText(&orbitalSpeedButton.text, font4, "Set Moon to Orbital Speed", renderer);
    orbitalSpeedButton.textRectx = 35;
    orbitalSpeedButton.textRecty = 430;

    LEObut = (Button){0};
    LEObut.buttonRect = (SDL_Rect){100, 585, 80, 45};
    LEObut.colorRect1= blue4;
    LEObut.colorBorders = blue5;
    updateCachedText(&LEObut.text, font2, "LEO", renderer);
    LEObut.textRectx = LEObut.buttonRect.x + 20;
    LEObut.textRecty = LEObut.buttonRect.y + 12;

    MEObut = (Button){0};
    MEObut.buttonRect = (SDL_Rect){190, 585, 80, 45};
    MEObut.colorRect1= green1;
    MEObut.colorBorders = green2;
    updateCachedText(&MEObut.text, font2, "MEO", renderer);
    MEObut.textRectx = MEObut.buttonRect.x + 20;
    MEObut.textRecty = MEObut.buttonRect.y + 12;

    GEObut = (Button){0};
    GEObut.buttonRect = (SDL_Rect){100, 640, 80, 45};
    GEObut.colorRect1= orange1;
    GEObut.colorBorders = orange2;
    updateCachedText(&GEObut.text, font2, "GEO", renderer);
    GEObut.textRecty = GEObut.buttonRect.y + 12;
    GEObut.textRectx = GEObut.buttonRect.x + 20;

    HEObut = (Button){0};
    HEObut.buttonRect = (SDL_Rect){190, 640, 80, 45};
    HEObut.colorRect1= red1;
    HEObut.colorBorders = red2;
    updateCachedText(&HEObut.text, font2, "*HEO", renderer);
    HEObut.textRectx = HEObut.buttonRect.x + 12;
    HEObut.textRecty = HEObut.buttonRect.y + 12;

    parabolicButton = (Button){0};
    parabolicButton.buttonRect = (SDL_Rect){100, 695, 170, 45};
    parabolicButton.colorRect1= cyan1;
    parabolicButton.colorBorders = cyan2;
    updateCachedText(&parabolicButton.text, font2, "Escape Vel", renderer);
    parabolicButton.textRectx = parabolicButton.buttonRect.x + 20;
    parabolicButton.textRecty = parabolicButton.buttonRect.y + 12;

    COMbut = (Button){0};
    COMbut.buttonRect = (SDL_Rect){20, 490, 320, 50};
    COMbut.colorRect1= red1;
    COMbut.colorBorders = red2;
    updateCachedText(&COMbut.text, font4, "Barycenter Indicator", renderer);
    COMbut.textRectx = COMbut.buttonRect.x + 10;
    COMbut.textRecty = COMbut.buttonRect.y + 14;

    trailsBut = (Button){0};
    trailsBut.buttonRect = (SDL_Rect){20, 730, 320, 50};
    trailsBut.colorRect1= purple1;
    trailsBut.colorBorders = purple2;
    updateCachedText(&trailsBut.text, font4, "Show Trails", renderer);
    trailsBut.textRectx = trailsBut.buttonRect.x + 80;
    trailsBut.textRecty = trailsBut.buttonRect.y + 14;


    // fps
    const int TARGET_FPS = 55;
    const double TICKS_PER_FRAME = (double)SDL_GetPerformanceFrequency() / TARGET_FPS;

    setOrbitalSpeed(&earth,  &moon, mode);


    while (running) {

        Uint64 frame_start = SDL_GetPerformanceCounter();

        // event handler
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }

            if (e.type == SDL_KEYDOWN) {

                // space
                if (e.key.keysym.sym == SDLK_SPACE) {
                    orbiting = !orbiting;     
                    
                    if (mode == 2 && moon.y == SCREEN_CENTER_Y * SCALE) {
                        earth.vy *= earth.vper;
                        moon.vy *= moon.vper;
                    }

                    if (mode == 1 && moon.y == SCREEN_CENTER_Y * SCALE) {
                        burning = true;
                        burningTime = counter;
                    }

                    
                // reset
                } if (e.key.keysym.sym == SDLK_r) {
                    reset(&earth, &moon, &massslider2, &radiusslider1, &radiusslider2, mode, tempAlt, false);
                    orbiting = false; 

                // hide ui
                } if (e.key.keysym.sym == SDLK_h) {
                    drawing = !drawing;
                
                } if (e.key.keysym.sym == SDLK_1 && mode == 2 && !orbiting) {
                    mode = 1;
                    modeToggle.colorRect1 = blue1;
                    modeToggle.colorRect2 = gray1;        
                    reset(&earth, &moon, &massslider2, &radiusslider1, &radiusslider2, mode, 250.0 * SCALE, true);

                } if (e.key.keysym.sym == SDLK_2 && mode == 1 && !orbiting) {
                    mode = 2;
                    modeToggle.colorRect1 = gray1;
                    modeToggle.colorRect2 = blue1;
                    reset(&earth, &moon, &massslider2, &radiusslider1, &radiusslider2, mode, 250.0 * SCALE, true);
                }

            
                }

            if (e.type == SDL_MOUSEBUTTONDOWN) {

                if (e.button.button == SDL_BUTTON_LEFT) {
                    SDL_GetMouseState(&mousex, &mousey); 

                    if (!orbiting) {

                        // mode toggle input
                        if (getInputToggle(&modeToggle) == 1 && mode == 2) {
                            mode = 1;
                            modeToggle.colorRect1 = blue1;
                            modeToggle.colorRect2 = gray1;

                            reset(&earth, &moon, &massslider2, &radiusslider1, &radiusslider2, mode, 250.0 * SCALE, true);

                        } if (getInputToggle(&modeToggle) == 2 && mode == 1) {
                            mode = 2;
                            modeToggle.colorRect1 = gray1;
                            modeToggle.colorRect2 = blue1;
                            
                            reset(&earth, &moon, &massslider2, &radiusslider1, &radiusslider2, mode, 250.0 * SCALE, true);
                        }
                    }

                    // Barycenter
                    if (mode == 2 && getInputButton(&COMbut) == true) {
                        COM = !COM;
                    }

                    // Trails
                    if (getInputButton(&trailsBut) == true) {
                        savingPoints = !savingPoints;
                    }
                }

            // Zoom by mousewheel
            } if (e.type == SDL_MOUSEWHEEL) {
                if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
                    if (e.wheel.y > 0) {

                        // Scrolled up
                        zoomFactor *= 1.2;

                    } else if (e.wheel.y < 0 && zoomFactor > SCALE / 20) {
                        
                        // Scrolled down
                        zoomFactor *= 0.8;
                    }
                }
            }
        }
    
    // User input
    Uint32 buttons = SDL_GetMouseState(&mousex, &mousex);
    
    // SLIDERS
    if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT) && !orbiting) {
        getInputSlider(&massslider1);
        getInputSlider(&massslider2);
        getInputSlider(&radiusslider1);
        getInputSlider(&radiusslider2);
        getInputSlider(&dtSlider);
        getInputSlider(&zoomSlider);
        tempAlt = moon.altitude;

        if (mode == 1) {
            getInputSlider(&altitudeSlider);
            getInputSlider(&velocitySlider);
        } if (mode == 2) {
            getInputSlider(&velocitySlider1);
            getInputSlider(&velocitySlider2);
            getInputSlider(&initialSeparationSlider);
        }

        // orbital speed burron
        if (getInputButton(&orbitalSpeedButton) && moon.y == SCREEN_CENTER_Y * SCALE) {
                
            if (mode == 1) {
                dx = moon.x -earth.x;
                dy = moon.y -earth.y;
                    
                moon.v = sqrt((G * earth.mass) / r);
                moon.vx = -moon.v * (dy/r);
                moon.vy = -moon.v * (dx/r);

            } else if (mode == 2) {
                setOrbitalSpeed(&earth, &moon, 2);
            }   
        }
    } 


    // precomputed orbits
    if (option == 2 && mode == 1 && moon.y == SCREEN_CENTER_Y * SCALE) {

    if (getInputButton(&LEObut)) {
        moon.altitude = 408000;
        set = true; // set orbital speed
    } else if (getInputButton(&MEObut)) {
        moon.altitude = 10000000;
        set = true; // set orbital speed
    } else if (getInputButton(&GEObut)) {
        moon.altitude = 35786000;
        set = true; // set orbital speed
    } else if (getInputButton(&HEObut)) {
        moon.altitude = 3000 * SCALE;
        moon.v = 1000;        
    } else if (getInputButton(&parabolicButton)) {
        moon.v = escapeVel;
    } 
    
    // escape button
    } else if (option == 2 && mode == 1 && getInputButton(&parabolicButton)) {
        moon.vx = escapeVel * (moon.vx / moon.v);
        moon.vy = escapeVel * (moon.vy / moon.v);
        moon.v = escapeVel;
    }
    
    // time warp slider
    if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT) && orbiting) {
        getInputSlider(&dtSlider);
    }
    
    // hohman transfers
    if (option == 1 && mode == 1 && moon.y != SCREEN_CENTER_Y * SCALE && startTransfer == 0) {
        if (getInputButton(&LEObut)) {
            targetR = 408000 + moon.radius + earth.radius;
            startTransfer = 1;
        } else if (getInputButton(&MEObut)) {
            targetR = 10000000 + moon.radius + earth.radius;
            startTransfer = 1;
        } else if (getInputButton(&GEObut)) {
            targetR = 35786000 + moon.radius + earth.radius;
            startTransfer = 1;
        } else if (getInputButton(&HEObut)) {
            targetR = 100000000 + moon.radius + earth.radius;
            startTransfer = 1;
        } 
    }
    
    // for only applying second burn once
    static bool apoapsis_fired = false;

    // for detecting apoapsis/periapsis moment for second burn
    static float prev_r = 0;

    if (startTransfer == 1 && orbiting && eccentricty < 0.1) {
        
        // initial r
        savedR = r;
        prev_r = r;  
        
        // gravitational parameter
        mu = G * earth.mass;
        
        // first delta v
        dv1 = sqrt(mu / r) * (sqrt( 2 * targetR / (r + targetR)) - 1);
        
        // normalize v
        moon.v = sqrt(moon.vx * moon.vx + moon.vy * moon.vy);
        
        // separte delta v 1 to its x and y components
        dv1_x = dv1 * (moon.vx / moon.v);
        dv1_y = dv1 * (moon.vy / moon.v);

        // apply delat v
        moon.vx += dv1_x;
        moon.vy += dv1_y;
        
        moon.v = sqrt(moon.vx * moon.vx + moon.vy * moon.vy);

        apoapsis_fired = false;
        
        // burning animation
        burning = true;
        burningTime = counter;
        
        // hohmann transfer stage 2
        startTransfer = 2;
    }
    
    // apoapsis alarm

    if (startTransfer == 2) {

        // point of burn 2
        bool at_extremum = (targetR > savedR) ? (prev_r > r) : (prev_r < r);
        
        if (at_extremum && !apoapsis_fired) {

            // delta v 2 for normalizing orbit
            dv2 = sqrt(mu / targetR) * (1 - sqrt( 2 * savedR / (savedR + targetR)));
            
            moon.v = sqrt(moon.vx * moon.vx + moon.vy * moon.vy);
            
            // separate it
            dv2_x = dv2 * (moon.vx / moon.v);
            dv2_y = dv2 * (moon.vy / moon.v);

            moon.vx += dv2_x;
            moon.vy += dv2_y;
            
            moon.v = sqrt(moon.vx * moon.vx + moon.vy * moon.vy);

            // stage 0 of hohmans transfer
            startTransfer = 0;

            apoapsis_fired = true;

            // burning animation
            burning = true;
            burningTime = counter;

            }  

            // previus r
            prev_r = r;
        }

    
    
    // keep burning for 2 seconds
    if (burning) {
        if (counter - burningTime > 100) {
            burning = false;
        }
    }


    // adjust sliders for normalizing data in simulation 
    massslider1.value = earth.mass;
    massslider1.ballx = 20 + (log(earth.mass) - log(massslider1.min)) / (log(massslider1.max) - log(massslider1.min)) * 150;
    massslider1.blue = (SDL_Rect){20, 150, massslider1.ballx-20, 15};
    snprintf(buffer, sizeof(buffer), "%.2e   %s", massslider1.value, massslider1.name);
    updateCachedText(&massslider1.id, font, buffer, renderer);

    massslider2.value = moon.mass;
    massslider2.ballx = 190 + (log(moon.mass) - log(massslider2.min)) / (log(massslider2.max) - log(massslider2.min)) * 150;
    massslider2.blue = (SDL_Rect){190, 150, massslider2.ballx -190, 15};
    snprintf(buffer, sizeof(buffer), "%.2e   %s", massslider2.value, massslider2.name);
    updateCachedText(&massslider2.id, font, buffer, renderer);
    
    radiusslider1.value = (int)(earth.radius / 1000);
    radiusslider1.blue = (SDL_Rect){20, 210, radiusslider1.ballx-20, 15};
    radiusslider1.ballx = 20 + (earth.radius - radiusslider1.min) / (radiusslider1.max - radiusslider1.min) * 150;
    snprintf(buffer, sizeof(buffer), "%.0f  %s", radiusslider1.value, radiusslider1.name);
    updateCachedText(&radiusslider1.id, font, buffer, renderer);
    
    radiusslider2.value = (int)(moon.radius / 1000);
    radiusslider2.ballx = 190 + (moon.radius - radiusslider2.min) / (radiusslider2.max - radiusslider2.min) * 150;
    radiusslider2.blue = (SDL_Rect){190, 210, radiusslider2.ballx -190, 15};
    snprintf(buffer, sizeof(buffer), "%.0f   %s", radiusslider2.value, radiusslider2.name);
    updateCachedText(&radiusslider2.id, font, buffer, renderer);
    
    
    // calculating altitude based on r
    if (orbiting) {
        moon.altitude = r - earth.radius - moon.radius;
    }
    
    // putting the moon on the correct alitude when starting
    else if (moon.y == SCREEN_CENTER_Y * SCALE && mode == 1){
        moon.x = earth.x + earth.radius +  moon.radius + moon.altitude;
        moon.vy = -moon.v;
    }

    
    
    altitudeSlider.value = moon.altitude;
    altitudeSlider.ballx = 20 + (altitudeSlider.value - altitudeSlider.min) / (altitudeSlider.max - altitudeSlider.min) * 300;
    if (altitudeSlider.ballx > 320) {altitudeSlider.ballx  = 320;}
    altitudeSlider.blue = (SDL_Rect){20, 310, altitudeSlider.ballx -20, 15};
    snprintf(buffer, sizeof(buffer), "%s: %.0f", altitudeSlider.name, altitudeSlider.value / 1000);
    updateCachedText(&altitudeSlider.id, font, buffer, renderer);
    
    
    velocitySlider.value = sqrt(moon.vx * moon.vx + moon.vy * moon.vy);
    velocitySlider.ballx = 20 + (velocitySlider.value - velocitySlider.min) / (velocitySlider.max - velocitySlider.min) * 300;
    if (velocitySlider.value > 1.2 * velocitySlider.max) {velocitySlider.ballx=360;}
    velocitySlider.blue = (SDL_Rect){20, 370, velocitySlider.ballx -20, 15};
    snprintf(buffer, sizeof(buffer), "%s: %.2f", velocitySlider.name, velocitySlider.value/1000);
    updateCachedText(&velocitySlider.id, font, buffer, renderer);

    velocitySlider1.value = earth.vper;
     if (velocitySlider1.value > 1.2 * velocitySlider1.max) {velocitySlider1.value = 1.2 * velocitySlider1.max;}
    velocitySlider1.ballx = 20 + (velocitySlider1.value - velocitySlider1.min) / (velocitySlider1.max - velocitySlider1.min) * 150;
    velocitySlider1.blue = (SDL_Rect){20, velocitySlider1.gray.y, velocitySlider1.ballx -20, 15};
    snprintf(buffer, sizeof(buffer), "%.0f%% %s", velocitySlider1.value*100, velocitySlider1.name);
    updateCachedText(&velocitySlider1.id, font, buffer, renderer);

    velocitySlider2.value = moon.vper;
    if (velocitySlider2.value > 1.2 * velocitySlider2.max) {velocitySlider2.value = 1.2 * velocitySlider2.max;}
    velocitySlider2.ballx = 190 + (velocitySlider2.value - velocitySlider2.min) / (velocitySlider2.max - velocitySlider2.min) * 150;
    velocitySlider2.blue = (SDL_Rect){190, velocitySlider2.gray.y, velocitySlider2.ballx -190, 15};
    snprintf(buffer, sizeof(buffer), "%.0f%% %s", velocitySlider2.value*100, velocitySlider2.name);
    updateCachedText(&velocitySlider2.id, font, buffer, renderer);


    initialSeparationSlider.value = r/ 1000.0;
    initialSeparationSlider.ballx = 20 + (r - initialSeparationSlider.min) / (initialSeparationSlider.max - initialSeparationSlider.min) * 300;
    if (initialSeparationSlider.value > 1.2 * initialSeparationSlider.max) {initialSeparationSlider.ballx = 20 + 1.2 * 300;}
    initialSeparationSlider.blue = (SDL_Rect){20, 320, initialSeparationSlider.ballx - 20, 15};
    snprintf(buffer, sizeof(buffer), "%s: %.0f", initialSeparationSlider.name, initialSeparationSlider.value);
    updateCachedText(&initialSeparationSlider.id, font, buffer, renderer);

    dtSlider.value = DT;
    dtSlider.ballx = 20 + (log(DT) - log(dtSlider.min)) / (log(dtSlider.max) - log(dtSlider.min)) * 300;
    dtSlider.blue = (SDL_Rect){20, 850, dtSlider.ballx - 20, 15};
    snprintf(buffer, sizeof(buffer), "%s: %.1fx", dtSlider.name, dtSlider.value*TARGET_FPS);
    updateCachedText(&dtSlider.id, font, buffer, renderer);

    zoomSlider.value = zoomFactor;
    if (zoomFactor > zoomSlider.max) {zoomFactor = zoomSlider.max;}
    if (zoomFactor < zoomSlider.min) {zoomFactor = zoomSlider.min;}
    zoomSlider.ballx = 1400 + (log(zoomFactor) - log(zoomSlider.min)) / (log(zoomSlider.max) - log(zoomSlider.min)) * 300;
    zoomSlider.blue = (SDL_Rect){1400, 870, zoomSlider.ballx - 1400, 15};
    snprintf(buffer, sizeof(buffer), "%s: 1px -> %.0fkm", zoomSlider.name, zoomSlider.value / 1000);
    updateCachedText(&zoomSlider.id, font, buffer, renderer);


    
    // physics

    // distance
    dx = moon.x -earth.x;
    dy = moon.y -earth.y;
    dvx = moon.vx - earth.vx;
    dvy = moon.vy - earth.vy;
    r = sqrt(dx*dx+dy*dy);

    moon.v = sqrt(moon.vx * moon.vx + moon.vy * moon.vy);

    
    // eccentricity
    if (mode == 1) {

        // gravitational parameter
        mu = G * earth.mass;

        // only accounting for moon's velocity (orbital energy)
        v = moon.v;
        angularMomentum = dx * moon.vy - dy * moon.vx;;

    } else {

        mu = G * (earth.mass + moon.mass);

        // calculating orbital energy based on the bodies on mode 2
        v = sqrt(dvx*dvx + dvy*dvy);
        angularMomentum = dx * dvy - dy * dvx;
    }
    
    // specific orbital energy
    orbitalEnergy = (v*v) / 2 - mu / r;    

    // applying eccentricty formula
    eccentricty = sqrt(1 + (2 * orbitalEnergy * angularMomentum * angularMomentum) / (mu * mu));

    mu = G * (earth.mass + moon.mass);
    orbitalEnergy = (v*v) / 2 - mu / r;
    semiMajorAxis = -mu / (2 * orbitalEnergy);     // half biggest distance of the elipse
    semiMinorAxis = semiMajorAxis * sqrt(1 - eccentricty*eccentricty);     // half smallest distance of the elipse
    periapsisAlt = semiMajorAxis * (1 - eccentricty);     // lowest point on the orbit
    apoapsisAlt = semiMajorAxis * (1 + eccentricty);     // highest point on the orbit
    orbitalPeriod = 2 * PI * sqrt((semiMajorAxis*semiMajorAxis*semiMajorAxis) / mu);     // time to complete an orbit




    // clasify the orbit based on eccentricty
    if (counter % 10 == 0) {
        if (eccentricty < 0.02) {
            updateCachedText(&orbitClassification, font4, "-> Circular", renderer);
        } else if (eccentricty < 0.50) {
            updateCachedText(&orbitClassification, font4, "-> Eliptical", renderer);
        } else if (eccentricty < 0.98) {
            updateCachedText(&orbitClassification, font4, "-> Highly Eliptical", renderer);
        } else if (eccentricty > 0.98 && eccentricty < 1.05) {
            updateCachedText(&orbitClassification, font4, "-> Parabolic Escape", renderer); // perfect escape
        } else if (eccentricty > 1.05) {
            updateCachedText(&orbitClassification, font4, "-> Hyperbolic Escape", renderer);
        }
    }

    // calculating the center of mass on the 2 body mode
    totalMass = moon.mass + earth.mass; 
    comx = (earth.mass * earth.x + moon.mass * moon.x) / totalMass;
    comy = (earth.mass * earth.y + moon.mass * moon.y) / totalMass;

    
    // set orbital speed
    if (set) {
        setOrbitalSpeedOneBody();
        set = false;
    }

    // ground collison
    if (r < moon.radius + earth.radius && orbiting) {
        orbiting = false;
        Mix_PlayChannel(-1, groundCollisionmp3, 0);
        updateCachedText(&orbitClassification, font4, "-> Ground Collsion", renderer);
        trigger = counter;
        play = true;
    }

    difference = SDL_GetTicks() - trigger;

    // wait a bit before collsion
    if (difference > 6000 && play) {
        reset(&earth, &moon, &massslider2, &radiusslider1, &radiusslider2, mode, 250.0*SCALE, true);
        play = false;
    }


    // calculate escap velocity
    escapeVel = sqrt((2 * G * earth.mass) / r);


    if (mode == 1) {

        // if reached escape velocity
        if (velocitySlider.value >= escapeVel && orbiting) {
            if (escapeVelocityTrigger) {
                escaped = true;
                Mix_PlayChannel(-1, escapeVelmp3, 0);  
                escapeVelocityTrigger = false;
            }

        } else if (!escapeVelocityTrigger){
            escapeVelocityTrigger = true;        
        }

        // if far away
        if (escaped && orbiting && r > 5000 * SCALE) {
            orbiting = !orbiting;
            escaped = false;
            reset(&earth, &moon, &massslider2, &radiusslider1, &radiusslider2, mode, 250.0 * SCALE, false);

        }
    }

    // fix planets in place if not orbiting
    if (!orbiting && moon.y == SCREEN_CENTER_Y * SCALE && mode == 2) {
        moon.x = SCREEN_CENTER_X * SCALE + separation / 2;
        earth.x = SCREEN_CENTER_X * SCALE - separation / 2;
    }


    // rk4 calcultions
    // The differnce between RK4 and normal euclidian calculations and what makes it much more accurate is the following:
    // RK4 calculates the next position 4 times, hence its name, and takes the average of them.
    // the first caultion is based on the derivative of current data
    // the second calculates only halfway through the timestep based on k1
    // the third one does the same based n k2 (only halfway through the timestep)
    // and finally the fourth one based o k3 through the hole time step


    if (orbiting) {
    
    derivative(moon, &k1_moon, earth, &k1_earth);
    
    tempMoon.x = moon.x + k1_moon.x * DT * 0.5;
    tempMoon.y = moon.y + k1_moon.y * DT * 0.5;
    tempMoon.vx = moon.vx + k1_moon.vx * DT * 0.5;
    tempMoon.vy = moon.vy + k1_moon.vy * DT * 0.5;
    tempMoon.mass = moon.mass;

    tempEarth.x = earth.x + k1_earth.x * DT * 0.5;
    tempEarth.y = earth.y + k1_earth.y * DT * 0.5;
    tempEarth.vx = earth.vx + k1_earth.vx * DT * 0.5;
    tempEarth.vy = earth.vy + k1_earth.vy * DT * 0.5;
    tempEarth.mass = earth.mass;

    derivative(tempMoon, &k2_moon, tempEarth, &k2_earth);

    tempMoon.x = moon.x + k2_moon.x * DT * 0.5;
    tempMoon.y = moon.y + k2_moon.y * DT * 0.5;
    tempMoon.vx = moon.vx + k2_moon.vx * DT * 0.5;
    tempMoon.vy = moon.vy + k2_moon.vy * DT * 0.5;
    tempMoon.mass = moon.mass;

    tempEarth.x = earth.x + k2_earth.x * DT * 0.5;
    tempEarth.y = earth.y + k2_earth.y * DT * 0.5;
    tempEarth.vx = earth.vx + k2_earth.vx * DT * 0.5;
    tempEarth.vy = earth.vy + k2_earth.vy * DT * 0.5;
    tempEarth.mass = earth.mass;
    
    derivative(tempMoon, &k3_moon, tempEarth, &k3_earth);

    tempMoon.x = moon.x + k3_moon.x * DT;
    tempMoon.y = moon.y + k3_moon.y * DT;
    tempMoon.vx = moon.vx + k3_moon.vx * DT;
    tempMoon.vy = moon.vy + k3_moon.vy * DT;
    tempMoon.mass = moon.mass;

    tempEarth.x = earth.x + k3_earth.x * DT;
    tempEarth.y = earth.y + k3_earth.y * DT;
    tempEarth.vx = earth.vx + k3_earth.vx * DT;
    tempEarth.vy = earth.vy + k3_earth.vy * DT;
    tempEarth.mass = earth.mass;

    derivative(tempMoon, &k4_moon, tempEarth, &k4_earth);



    // take average
    double avgxMoon, avgyMoon, avgvxMoon, avgvyMoon;
    double avgxEarth, avgyEarth, avgvxEarth, avgvyEarth;


    avgxMoon = (k1_moon.x + 2.0*k2_moon.x + 2.0*k3_moon.x + k4_moon.x) / 6.0;
    avgyMoon = (k1_moon.y + 2.0*k2_moon.y + 2.0*k3_moon.y + k4_moon.y) / 6.0;
    avgvxMoon = (k1_moon.vx + 2.0*k2_moon.vx + 2.0*k3_moon.vx + k4_moon.vx) / 6.0;
    avgvyMoon = (k1_moon.vy + 2.0*k2_moon.vy + 2.0*k3_moon.vy + k4_moon.vy) / 6.0;

    avgxEarth = (k1_earth.x + 2.0*k2_earth.x + 2.0*k3_earth.x + k4_earth.x) / 6.0;
    avgyEarth = (k1_earth.y + 2.0*k2_earth.y + 2.0*k3_earth.y + k4_earth.y) / 6.0;
    avgvxEarth = (k1_earth.vx + 2.0*k2_earth.vx + 2.0*k3_earth.vx + k4_earth.vx) / 6.0;
    avgvyEarth = (k1_earth.vy + 2.0*k2_earth.vy + 2.0*k3_earth.vy + k4_earth.vy) / 6.0;
    

    // aply it
    moon.x += avgxMoon * DT;
    moon.y += avgyMoon * DT;
    moon.vx += avgvxMoon * DT;
    moon.vy += avgvyMoon * DT;
    
    earth.x += avgxEarth * DT;
    earth.y += avgyEarth * DT;
    earth.vx += avgvxEarth * DT;
    earth.vy += avgvyEarth * DT;

    moon.a  = a;
    earth.a  = a2;
    
    // get points based on the moon's x and y

    // how far away is last point
    lastPointSepartion = sqrt((moonTrail[(trailLengthMoon-99)% 100].x - moon.x)*(moonTrail[(trailLengthMoon-99)% 100].x - moon.x) + (moonTrail[(trailLengthMoon-99)% 100].y - moon.y)*(moonTrail[(trailLengthMoon-1)% 100].y - moon.y));
    if (lastPointSepartion > 30 * SCALE) { // if last point is at least 30 px away 
        moonTrail[trailLengthMoon % 100] = (SDL_Point){moon.x, moon.y}; // save it
        trailLengthMoon ++;
    }
    

    // asame for the trail of the earth (only in mode 2)
    lastPointSepartion = sqrt((earthTrail[(trailLengthMoon-99)% 100].x - earth.x)*(earthTrail[(trailLengthMoon-99)% 100].x - earth.x) + (earthTrail[(trailLengthMoon-99)% 100].y - earth.y)*(earthTrail[(trailLengthMoon-1)% 100].y - earth.y));
    if (mode == 2 && lastPointSepartion / SCALE > 30) {
        earthTrail[trailLengthEarth % 100] = (SDL_Point){earth.x, earth.y};
        trailLengthEarth++;
    } 

    }

    // counter used for many things
    if (orbiting) {
        counter++;
    }

    // keep earth glued on mode 1
    if (mode == 1) {
        earth.x = SCREEN_CENTER_X * SCALE;
        earth.y = SCREEN_CENTER_Y * SCALE;
    }

    // change mode with the toggle
    if (getInputToggle(&orbitsToggle) == 1 && mode == 1) {
        option = 1;
    } else if (getInputToggle(&orbitsToggle) == 2 && mode == 1) {
        option = 2;
    }

    // change the colors of mode toggle
    if (option == 1) {
        orbitsToggle.colorRect1 = blue1;
        orbitsToggle.colorRect2 = gray1;
    } else if (option == 2) {
        orbitsToggle.colorRect1 = gray1;
        orbitsToggle.colorRect2 = blue1;
    }

    
    // First clear
    SDL_RenderClear(renderer);

    // DRAW
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);


    // trails
    if (savingPoints) {
        for (int i = 0; i < 100; i++) {
            SDL_RenderDrawPoint(renderer, 900 + (moonTrail[i].x - SCREEN_CENTER_X * SCALE) / zoomFactor, 450 + (moonTrail[i].y - SCREEN_CENTER_Y * SCALE) / zoomFactor);
        }

        if (mode == 2) {
            for (int i = 0; i < 100; i++) {
                SDL_RenderDrawPoint(renderer, 900 + (earthTrail[i].x - SCREEN_CENTER_X * SCALE) / zoomFactor, 450 + (earthTrail[i].y - SCREEN_CENTER_Y * SCALE) / zoomFactor);
            }
        }
    }

    // angle based on direction of travel
    float angle_rad = atan2(moon.vy, moon.vx);
    float angle_deg = angle_rad * (180.0f / PI);

    // on/off engine engines
    if (mode == 1 & burning) {
        renderImageRotated(renderer, raptorONsurface, moon.drawx, moon.drawy, (moon.radius / zoomFactor), (moon.radius / zoomFactor) * 6.16, angle_deg+90);
    } else if (mode == 1) {
        renderImageRotated(renderer, raptorOFFsurface, moon.drawx, moon.drawy, (moon.radius / zoomFactor), (moon.radius / zoomFactor) * 2.02, angle_deg+90);
    } 
    
    // I couldnt solve a bug where a white point would be drawm at 0, 0 so instead i draw it black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawPoint(renderer, 900 + (-SCREEN_CENTER_X * SCALE) / zoomFactor, 450 + (-SCREEN_CENTER_Y * SCALE) / zoomFactor);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);


    // draw cordinates
    earth.drawx = 900 + (earth.x - SCREEN_CENTER_X * SCALE) / zoomFactor;
    earth.drawy = 450 + (earth.y - SCREEN_CENTER_Y * SCALE) / zoomFactor;

    moon.drawx = 900 + (moon.x - SCREEN_CENTER_X * SCALE) / zoomFactor;
    moon.drawy = 450 + (moon.y - SCREEN_CENTER_Y * SCALE) / zoomFactor;


    // draw the planets
    drawCircle(renderer, earth.drawx, earth.drawy, earth.radius / zoomFactor, white);
    drawCircle(renderer, moon.drawx, moon.drawy, moon.radius / zoomFactor, white);

    // draw barycenter
    if (mode == 2 && COM) {
        drawCircle(renderer, 900 + (comx-SCREEN_CENTER_X * SCALE) / zoomFactor, 450 + (comy-SCREEN_CENTER_Y * SCALE) / zoomFactor, 2*SCALE / (zoomFactor / 4), red);
    }

    // if show UI
    if (drawing) {
    
    if (orbiting) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // green
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
    }

    // signal
    SDL_RenderFillRect(renderer, &signal);

    // Text

    // HEO can mean higly eliptical orbit and high earth orbit, I give it difrent meanings on optio 1 and 2
    if (mode == 1) {
        if (option == 2) {
            render_text(&orbits, renderer, 20, 545);
            updateCachedText(&heomessage, font3, "*Higly Eliptical Orbit", renderer);
        } else {
            render_text(&hohmannText, renderer, 20, 545);
            updateCachedText(&heomessage, font3, "*High Earth Orbit", renderer);
            render_text(&sorryText, renderer, 30, 710); // sorry :(

        }
        render_text(&heomessage, renderer, 275, 660);    
    }

    // info
    render_text(&modeText, renderer, 10, 15);
    render_text(&bodies, renderer, 15, 75);
    render_text(&pressR, renderer, 1520, 10);
    render_text(&pressH, renderer, 1490, 40);
    render_text(&pressS, renderer, 650, 10);
    render_text(&realTime, renderer, 1440, 100);
    render_text(&orbitClassSentence, renderer, 1440, 310);
    render_text(&orbitClassification, renderer, 1440, 350);
    render_text(&orbitalParameters, renderer, 1440, 440);


    if(counter % 2 == 0) {

    // real-time data
    if (mode == 1) {
        snprintf(moonxyBuffer, sizeof(moonxyBuffer), "1. Moon X: %.1e Y: %.1e", moon.x, moon.y);
        snprintf(moonvxvyaBuffer, sizeof(moonvxvyaBuffer), "2. Moon Vx: %.1e Vy: %.1e A: %.2f", moon.vx  / (1/DT), moon.vy  / (1/DT), moon.a  / (1/DT));
        snprintf(timePassedBuffer, sizeof(timePassedBuffer), "3. Elapsed Time: %.2f",(double)counter / (double)TARGET_FPS);

    } else if (mode == 2) {
        snprintf(earthxyBuffer, sizeof(earthxyBuffer), "1. Planet 1 X: %.1e Y: %.1e", earth.x, earth.y);
        snprintf(earthvxvyaBuffer, sizeof(earthvxvyaBuffer), "2. Planet 1 Vx: %.1e Vy: %.1e A: %.2f", earth.vx / (1/DT), earth.y / (1/DT), earth.a / (1/DT));
        snprintf(moonxyBuffer, sizeof(moonxyBuffer), "3. Planet 2 X: %.1e Y: %.1e", moon.x, moon.y);
        snprintf(moonvxvyaBuffer, sizeof(moonvxvyaBuffer), "4. Planet 2 Vx: %.1e Vy: %.1e A: %.2f", moon.vx / (1/DT), moon.y / (1/DT), moon.a / (1/DT));
        snprintf(timePassedBuffer, sizeof(timePassedBuffer), "5. Elapsed Time: %.2f", (double)counter / (double)TARGET_FPS);

    }

    updateCachedText(&timePassed, specialFont, timePassedBuffer, renderer);
    
    if (mode == 2) {
        updateCachedText(&earthxy, specialFont, earthxyBuffer, renderer);
        updateCachedText(&earthvxvya, specialFont, earthvxvyaBuffer, renderer);

    } 

    updateCachedText(&moonxy, specialFont, moonxyBuffer, renderer);
    updateCachedText(&moonvxvya, specialFont, moonvxvyaBuffer, renderer);

    
    }

    // render real-time data
    if (mode == 1) {
        render_text(&moonxy , renderer, 1400, 140);
        render_text(&moonvxvya , renderer, 1400, 170);
        render_text(&timePassed , renderer, 1400, 200);
    } else if (mode == 2) {
        render_text(&earthxy, renderer, 1390, 140);
        render_text(&earthvxvya, renderer, 1390, 170);
        render_text(&moonxy , renderer, 1390, 200);
        render_text(&moonvxvya , renderer, 1390, 230);
        render_text(&timePassed , renderer, 1390, 260);

    }

    // orbit data
    if (!orbiting || counter % 10 == 0) { // update if not orbiting or after 1/5 second
        snprintf(eccentrictyBuffer, sizeof(eccentrictyBuffer), "1. Eccentricty: %.3f", eccentricty);
        snprintf(semiMajorAxisBuffer, sizeof(semiMajorAxisBuffer), "2. Semi-Major Axis: %.0fkm", semiMajorAxis / 1000);
        snprintf(semiMinorAxisBuffer, sizeof(semiMinorAxisBuffer), "3. Semi-Minor Axis: %.0fkm", semiMinorAxis / 1000);
        snprintf(periapsisBuffer, sizeof(periapsisBuffer), "4. Periapsis: %.0fkm", periapsisAlt / 1000);
        snprintf(apoapsisBuffer, sizeof(apoapsisBuffer), "5. Apoapsis: %.0fkm", apoapsisAlt / 1000);
        snprintf(muBuffer, sizeof(muBuffer), "6. Gravitational Parameter: %.3e", mu);
        snprintf(orbitalEnergyBuffer, sizeof(orbitalEnergyBuffer), "7. Specific Orbital Energy: %.0fj/kg", orbitalEnergy);
        snprintf(angularMomentumBuffer, sizeof(angularMomentumBuffer), "8. Ang. Momentum: %.2ekgm2/s", angularMomentum);
        snprintf(orbitalPeriodBuffer, sizeof(orbitalPeriodBuffer), "9. Orbital Period: %.2fh", orbitalPeriod / 3600);
        snprintf(escapeVelBuffer, sizeof(escapeVelBuffer), "10. Escape Velocity: %.2fkm/s", escapeVel / 1000);


        updateCachedText(&eccentrictyText, specialFont, eccentrictyBuffer, renderer);
        updateCachedText(&semiMajorAxisText, specialFont, semiMajorAxisBuffer, renderer);
        updateCachedText(&semiMinorAxisText, specialFont, semiMinorAxisBuffer, renderer);
        updateCachedText(&periapssisText, specialFont, periapsisBuffer, renderer);
        updateCachedText(&apoapsisText, specialFont, apoapsisBuffer, renderer);
        updateCachedText(&muText, specialFont, muBuffer, renderer);
        updateCachedText(&orbitalEnergyText, specialFont, orbitalEnergyBuffer, renderer);
        updateCachedText(&angularMomentumText, specialFont, angularMomentumBuffer, renderer);
        updateCachedText(&orbitalPeriodText, specialFont, orbitalPeriodBuffer, renderer);
        updateCachedText(&escapeVelText, specialFont, escapeVelBuffer, renderer);
    }
    
    // rener the orbital data
    render_text(&eccentrictyText, renderer, 1400, 480);
    render_text(&semiMajorAxisText, renderer, 1400, 510);
    render_text(&semiMinorAxisText, renderer, 1400, 540);
    render_text(&periapssisText, renderer, 1400, 570);
    render_text(&apoapsisText, renderer, 1400, 600);
    render_text(&muText, renderer, 1400, 630);
    render_text(&orbitalEnergyText, renderer, 1400, 660);
    render_text(&angularMomentumText, renderer, 1400, 690);
    render_text(&orbitalPeriodText, renderer, 1400, 720);
    render_text(&escapeVelText, renderer, 1400, 750);
    
    // warning of a high dt 
    if (DT >  1000) {
        render_text(&warning, renderer, dtSlider.gray.x + dtSlider.gray.w + 15, dtSlider.gray.y - 5);
    }

    // density text
    densityEarth = earth.mass / (4.0/3.0 * PI * earth.radius * earth.radius * earth.radius);
    densityMoon = moon.mass / (4.0/3.0 * PI * moon.radius * moon.radius * moon.radius) ;
    snprintf(buffer, sizeof(buffer), "%.2f Density (kg/m3) %.2f", densityEarth, densityMoon);
    updateCachedText(&densityT, font, buffer, renderer);
    render_text(&densityT, renderer, 10, 250);


    // Toggles
    drawToggle(&modeToggle, renderer, font, mode);
    if (mode == 1) {drawToggle(&orbitsToggle, renderer, font, mode);}

    // Sliders
    drawSlider(renderer, massslider1, font);
    drawSlider(renderer, massslider2, font);
    drawSlider(renderer, radiusslider1, font);
    drawSlider(renderer, radiusslider2, font);
    drawSlider(renderer, dtSlider, font);
    drawSlider(renderer, zoomSlider, font);
    
    if (mode == 1) {
        drawSlider(renderer, altitudeSlider, font);
        drawSlider(renderer, velocitySlider, font);
    } if (mode == 2) {
        drawSlider(renderer, initialSeparationSlider, font);
        drawSlider(renderer, velocitySlider1, font);
        drawSlider(renderer, velocitySlider2, font);
    }
    
    
    // Buttons
    if (mode == 1)  {
        orbitalSpeedButton.buttonRect.w = 406;
        updateCachedText(&orbitalSpeedButton.text, font4, "Set Moon to Orbital Speed", renderer);
        trailsBut.buttonRect.y = 760;
        
        if (option == 2) {
            drawButton(renderer, &parabolicButton);
        }

        drawButton(renderer, &LEObut);
        drawButton(renderer, &MEObut);
        drawButton(renderer, &GEObut);
        drawButton(renderer, &HEObut);

    } else if (mode == 2) {
        orbitalSpeedButton.buttonRect.w = 450;
        trailsBut.buttonRect.y = 565;

        updateCachedText(&orbitalSpeedButton.text, font4, "Set Planets to Orbital Speed", renderer);
        drawButton(renderer, &COMbut);
    }  
    
    drawButton(renderer, &orbitalSpeedButton);
    trailsBut.textRecty = trailsBut.buttonRect.y + 14;
    drawButton(renderer, &trailsBut);
    
    }
    
    // Finally update
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderPresent(renderer);

    // wait if going above 50fps
    Uint64 frame_ticks = SDL_GetPerformanceCounter() - frame_start;
    if (frame_ticks < TICKS_PER_FRAME) {
            double remaining_ticks = TICKS_PER_FRAME - frame_ticks;
            Uint32 delay_ms = (Uint32)((remaining_ticks * 1000.0) / SDL_GetPerformanceFrequency());
            SDL_Delay(delay_ms);
    }
    
    }

    // free sounds
    Mix_FreeChunk(escapeVelmp3);
    Mix_FreeChunk(groundCollisionmp3);
    Mix_CloseAudio();
    
    // exit
    SDL_DestroyRenderer(renderer);  // cleanup after loop exits
    SDL_DestroyWindow(win);
    SDL_Quit();


    return 0;


}

