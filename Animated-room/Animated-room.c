#include "raylib.h"
#include "rlgl.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define C(r,g,b) (Color){r,g,b,255}
typedef enum { ROOM, DESK, PAPER, SHELF, BOOK, WARDROBE, FRAME, ENDING } View;
typedef enum { NO_OBJECT, PAPER_OBJ, SHELF_OBJ, BOOK_OBJ, WARDROBE_OBJ, CLOTHES_OBJ, FRAME_OBJ, KEY_OBJ, DOOR_OBJ, FIRST_DOOR_OBJ, SECOND_DOOR_OBJ } Object;
typedef struct { Object id; BoundingBox box; } HitTarget;
typedef struct { Rectangle left,right,back; } Controls;

static const Color wall={226,221,207,255}, wallLight={238,234,224,255}, trim={116,91,69,255};
static const Color wood={139,101,70,255}, woodLight={174,131,91,255}, woodDark={91,67,49,255};
static const Color floorColor={174,139,102,255}, fabric={224,218,204,255}, fabricLight={243,239,226,255};
static const char *viewNames[4]={"FRONT","RIGHT","BACK","LEFT"};
static const Camera3D roomCameras[4]={
    {{0,1.68f,-2.50f},{0,1.42f,3.25f},{0,1,0},80,CAMERA_PERSPECTIVE},
    {{2.30f,1.66f,-.90f},{-3.75f,1.42f,2.50f},{0,1,0},84,CAMERA_PERSPECTIVE},
    {{0,1.68f,2.50f},{0,1.42f,-3.25f},{0,1,0},80,CAMERA_PERSPECTIVE},
    {{-2.30f,1.66f,0},{4.35f,1.42f,0},{0,1,0},80,CAMERA_PERSPECTIVE}
};
static const HitTarget targets[]={
    {PAPER_OBJ,{{-4.84f,.54f,2.60f},{-4.42f,.57f,2.96f}}},
    {SHELF_OBJ,{{-4.98f,.05f,-2.72f},{-4.02f,2.55f,.12f}}},
    {BOOK_OBJ,{{-4.27f,1.53f,-1.02f},{-3.83f,1.97f,-.56f}}},
    {WARDROBE_OBJ,{{1.02f,.05f,-3.98f},{4.82f,2.80f,-2.80f}}},
    {FRAME_OBJ,{{3.68f,.75f,1.30f},{3.91f,1.43f,1.94f}}},
    {CLOTHES_OBJ,{{1.12f,.35f,-3.52f},{2.84f,2.25f,-3.28f}}},
    {KEY_OBJ,{{3.69f,.61f,-3.30f},{4.22f,.91f,-3.14f}}},
    {DOOR_OBJ,{{-4.35f,.05f,-3.98f},{-2.78f,2.35f,-3.60f}}}
};
static View view=PAPER, paperReturn=DESK, returnView=ROOM;
static int currentView=0,clothesClicks=0;
static int endingStage=0;
static float doorSlide=0,secondDoorSlide=0,messageTime=0,escapeDoorAngle=0,endingStageTime=0;
static Camera3D camera={0};
static bool sectionOpen=false,secondSectionOpen=false,codeUnlocked=false,keyCollected=false,escapeOpening=false;
static char code[5]="",codeMessage[96]="",message[96]="";

static void box(Vector3 p,Vector3 s,Color c){DrawCube(p,s.x,s.y,s.z,c);}
static void drawTextAt(const char *text,float x,float y,int size,Color c){DrawText(text,(int)x,(int)y,size,c);}
static void drawRectangleAt(float x,float y,float width,float height,Color c){DrawRectangle((int)x,(int)y,(int)width,(int)height,c);}
static void drawLineAt(float x1,float y1,float x2,float y2,Color c){DrawLine((int)x1,(int)y1,(int)x2,(int)y2,c);}
static void drawCircleAt(float x,float y,float radius,Color c){DrawCircle((int)x,(int)y,radius,c);}

static void drawEscapeDoor(void)
{
    box((Vector3){-4.14f,1.16f,-3.82f},(Vector3){.09f,2.34f,.13f},woodDark);
    box((Vector3){-2.98f,1.16f,-3.82f},(Vector3){.09f,2.34f,.13f},woodDark);
    box((Vector3){-3.56f,2.2875f,-3.82f},(Vector3){1.25f,.085f,.13f},woodDark);
    box((Vector3){-3.56f,.0325f,-3.82f},(Vector3){1.25f,.085f,.13f},woodDark);

    /* Positive Y rotation sends the panel outward through the back wall. */
    rlPushMatrix();
    rlTranslatef(-4.10f,0,-3.73f);
    rlRotatef(escapeDoorAngle,0,1,0);
    rlTranslatef(.54f,1.16f,0);
    DrawCube((Vector3){0,0,0},1.08f,2.17f,.07f,C(157,115,80));
    rlPopMatrix();
}

static void drawShell(void)
{
    box((Vector3){0,-.13f,0},(Vector3){10,.26f,8},floorColor);
    for(int i=-7;i<=7;i++){float z=i*.48f;DrawLine3D((Vector3){-4.92f,.006f,z},(Vector3){4.92f,.006f,z},C(139,108,77));}
    box((Vector3){-3.3f,1.6f,3.96f},(Vector3){3.4f,3.2f,.16f},wall);
    box((Vector3){3.3f,1.6f,3.96f},(Vector3){3.4f,3.2f,.16f},wall);
    box((Vector3){0,.5f,3.96f},(Vector3){3.2f,1,.16f},wall);
    box((Vector3){0,2.96f,3.96f},(Vector3){3.2f,.48f,.16f},wall);
    box((Vector3){-4.56f,1.6f,-3.96f},(Vector3){.88f,3.2f,.16f},wall);
    box((Vector3){1,1.6f,-3.96f},(Vector3){8,3.2f,.16f},wall);
    box((Vector3){-3.56f,2.79f,-3.96f},(Vector3){1.12f,.82f,.16f},wall);
    box((Vector3){-4.96f,1.6f,0},(Vector3){.16f,3.2f,8},wall);
    box((Vector3){4.96f,1.6f,2.8f},(Vector3){.16f,3.2f,2.4f},wall);
    box((Vector3){4.96f,1.6f,-3.2f},(Vector3){.16f,3.2f,1.6f},wall);
    box((Vector3){4.96f,2.8f,-1.4f},(Vector3){.16f,.8f,2},wall);
    box((Vector3){0,.12f,3.84f},(Vector3){9.8f,.2f,.06f},trim);
    box((Vector3){0,.12f,-3.84f},(Vector3){9.8f,.2f,.06f},trim);
    box((Vector3){-4.84f,.12f,0},(Vector3){.06f,.2f,7.6f},trim);
    box((Vector3){4.84f,.12f,2.8f},(Vector3){.06f,.2f,2.25f},trim);
    box((Vector3){0,3.12f,3.82f},(Vector3){9.85f,.12f,.14f},wallLight);
    box((Vector3){0,3.12f,-3.82f},(Vector3){9.85f,.12f,.14f},wallLight);
    box((Vector3){-4.82f,3.12f,0},(Vector3){.14f,.12f,7.7f},wallLight);
    box((Vector3){4.82f,3.12f,2.8f},(Vector3){.14f,.12f,2.25f},wallLight);
}
static void drawWindow(void)
{
    box((Vector3){0,1.92f,3.84f},(Vector3){3.12f,1.66f,.1f},woodDark);
    box((Vector3){0,1.92f,3.78f},(Vector3){2.88f,1.42f,.035f},C(173,211,222));
    box((Vector3){0,1.92f,3.75f},(Vector3){.055f,1.42f,.035f},fabricLight);
    box((Vector3){0,1.92f,3.745f},(Vector3){2.88f,.055f,.035f},fabricLight);
    box((Vector3){0,1.08f,3.74f},(Vector3){3.32f,.12f,.2f},woodLight);
    box((Vector3){-1.74f,1.96f,3.82f},(Vector3){.58f,1.72f,.13f},C(194,181,157));
    box((Vector3){1.74f,1.96f,3.82f},(Vector3){.58f,1.72f,.13f},C(194,181,157));
}
static void drawDesk(void)
{
    box((Vector3){-4.42f,.43f,2.73f},(Vector3){.84f,.12f,1.78f},woodLight);
    box((Vector3){-4.42f,.5f,2.73f},(Vector3){.72f,.08f,1.63f},wood);
    const float x[2]={-4.76f,-4.08f},z[2]={1.98f,3.48f};
    for(int i=0;i<2;i++)for(int j=0;j<2;j++)box((Vector3){x[i],.23f,z[j]},(Vector3){.1f,.46f,.1f},woodDark);
    /* The desktop top is y=.54; each prop now rests on it. */
    box((Vector3){-4.63f,.551f,2.78f},(Vector3){.4f,.014f,.32f},C(244,240,225));
    DrawCylinder((Vector3){-4.61f,.58f,2.05f},.15f,.13f,.08f,16,woodDark);
    DrawCylinder((Vector3){-4.61f,.82f,2.05f},.035f,.045f,.4f,12,C(75,75,72));
    DrawCylinder((Vector3){-4.61f,1.07f,2.05f},.22f,.11f,.2f,16,C(210,187,143));
    /* A simple mug and handle, placed near the opposite front corner. */
    DrawCylinder((Vector3){-4.18f,.66f,2.30f},.12f,.14f,.24f,20,C(231,226,210));
    DrawCylinder((Vector3){-4.18f,.783f,2.30f},.095f,.095f,.012f,20,C(86,68,51));
    box((Vector3){-4.02f,.67f,2.30f},(Vector3){.08f,.13f,.055f},C(231,226,210));
}
static void drawBedAndFrame(void)
{
    box((Vector3){2.83f,.29f,3},(Vector3){3.82f,.38f,1.74f},woodDark);
    box((Vector3){2.83f,.53f,3},(Vector3){3.72f,.18f,1.66f},wood);
    box((Vector3){2.83f,.69f,3},(Vector3){3.62f,.28f,1.58f},fabricLight);
    box((Vector3){3,.85f,3},(Vector3){2.35f,.1f,1.42f},C(194,199,183));
    box((Vector3){1.67f,.86f,3},(Vector3){.76f,.09f,1.45f},fabric);
    box((Vector3){4.06f,.94f,2.63f},(Vector3){.48f,.15f,.58f},C(250,247,238));
    box((Vector3){4.06f,.94f,3.35f},(Vector3){.48f,.15f,.58f},C(250,247,238));
    box((Vector3){4.73f,.95f,3},(Vector3){.16f,1.05f,1.84f},woodDark);
    box((Vector3){4.27f,.39f,1.6f},(Vector3){.78f,.12f,.76f},woodLight);
    box((Vector3){4.27f,.6f,1.6f},(Vector3){.68f,.3f,.66f},wood);
    box((Vector3){3.84f,1.01f,1.62f},(Vector3){.09f,.46f,.53f},woodDark);
    box((Vector3){3.78f,1.01f,1.62f},(Vector3){.025f,.36f,.42f},C(105,168,190));
    /* Small illustrated side profile of a race car inside the tabletop frame. */
    box((Vector3){3.758f,.99f,1.62f},(Vector3){.018f,.13f,.31f},C(48,67,79));
    box((Vector3){3.735f,1.01f,1.62f},(Vector3){.02f,.055f,.24f},C(211,62,48));
    box((Vector3){3.735f,1.08f,1.62f},(Vector3){.025f,.035f,.12f},C(239,238,220));
    box((Vector3){3.735f,1,1.48f},(Vector3){.025f,.12f,.035f},C(32,36,39));
    box((Vector3){3.735f,1,1.76f},(Vector3){.025f,.12f,.035f},C(32,36,39));
    box((Vector3){3.72f,1.17f,1.62f},(Vector3){.03f,.035f,.32f},C(239,238,220));
}
static void drawShelf(void)
{
    box((Vector3){-4.63f,1.25f,-1.33f},(Vector3){.74f,2.5f,2.66f},woodDark);
    box((Vector3){-4.38f,1.25f,-1.33f},(Vector3){.12f,2.38f,2.52f},wood);
    const float ys[6]={.12f,.6f,1.08f,1.56f,2.04f,2.42f};
    for(int i=0;i<6;i++)box((Vector3){-4.2f,ys[i],-1.33f},(Vector3){.72f,.075f,2.52f},woodLight);
    const Color colors[8]={{71,91,104,255},{166,103,72,255},{103,119,91,255},{191,164,116,255},{119,94,117,255},{66,84,89,255},{183,128,87,255},{119,126,120,255}};
    for(int s=0;s<4;s++)for(int b=0;b<(s==2?6:7);b++){
        float z=-2.32f+b*.32f+(s%2)*.04f,h=.3f+(b%3)*.045f;
        box((Vector3){-4.11f,.6f+s*.48f+h*.5f,z},(Vector3){.3f,h,.23f},colors[(b+s*2)%8]);
    }
    box((Vector3){-3.952f,1.70f,-.79f},(Vector3){.018f,.25f,.045f},C(184,71,54));
}
static void drawPiggy(void)
{
    DrawSphere((Vector3){-4.02f,1.31f,-2.22f},.23f,C(220,151,157));
    DrawSphere((Vector3){-3.82f,1.34f,-2.22f},.14f,C(231,171,174));
    DrawSphere((Vector3){-3.94f,1.54f,-2.36f},.085f,C(220,151,157));
    DrawSphere((Vector3){-3.94f,1.54f,-2.08f},.085f,C(220,151,157));
}
static void drawClothes(void)
{
    DrawCylinder((Vector3){1.98f,2.39f,-3.48f},.025f,.025f,1.55f,12,woodLight);
    const Color colors[5]={{70,104,126,255},{142,75,65,255},{112,122,89,255},{198,174,134,255},{90,87,113,255}};
    for(int i=0;i<5;i++){
        float x=1.34f+i*.29f;
        DrawLine3D((Vector3){x,2.35f,-3.48f},(Vector3){x+.1f,2.15f,-3.48f},woodLight);
        box((Vector3){x+.1f,1.7f,-3.4f},(Vector3){.25f,.92f,.15f},colors[i]);
        box((Vector3){x+.1f,2.14f,-3.4f},(Vector3){.32f,.15f,.15f},colors[i]);
        box((Vector3){x+.1f,1.19f,-3.4f},(Vector3){.2f,.12f,.13f},colors[i]);
    }
}
static void drawWardrobe(void)
{
    box((Vector3){2.9f,1.28f,-3.78f},(Vector3){3.72f,2.5f,.18f},woodDark);
    box((Vector3){1.04f,1.28f,-3.42f},(Vector3){.16f,2.5f,.62f},wood);
    box((Vector3){4.76f,1.28f,-3.42f},(Vector3){.16f,2.5f,.62f},wood);
    box((Vector3){2.9f,2.53f,-3.42f},(Vector3){3.88f,.14f,.62f},woodLight);
    box((Vector3){2.9f,.07f,-3.42f},(Vector3){3.88f,.14f,.62f},woodLight);
    box((Vector3){2.9f,1.29f,-3.45f},(Vector3){.08f,2.28f,.48f},woodDark);
    drawClothes();
    /* The right compartment has real depth and simple storage shelves. */
    box((Vector3){3.78f,1.28f,-3.68f},(Vector3){1.68f,2.22f,.035f},C(110,78,56));
    box((Vector3){3.02f,1.28f,-3.34f},(Vector3){.07f,2.2f,.62f},woodDark);
    box((Vector3){4.54f,1.28f,-3.34f},(Vector3){.07f,2.2f,.62f},woodDark);
    box((Vector3){3.78f,.61f,-3.34f},(Vector3){1.48f,.075f,.62f},woodLight);
    box((Vector3){3.78f,1.31f,-3.34f},(Vector3){1.48f,.075f,.62f},woodLight);
    box((Vector3){3.78f,2.02f,-3.34f},(Vector3){1.48f,.075f,.62f},woodLight);
    box((Vector3){3.34f,.78f,-3.38f},(Vector3){.43f,.26f,.34f},C(120,100,75));
    box((Vector3){4.15f,1.48f,-3.38f},(Vector3){.48f,.27f,.36f},C(170,139,103));
    box((Vector3){3.45f,2.18f,-3.38f},(Vector3){.55f,.25f,.36f},C(126,137,111));
    if(!secondSectionOpen)box((Vector3){3.78f,1.30f,-2.94f},(Vector3){1.72f,2.26f,.045f},C(174,132,93));
    box((Vector3){3.78f-secondDoorSlide*1.68f,1.30f,-2.91f},(Vector3){1.55f,2.10f,.025f},C(166,126,87));
    float x=1.98f+1.68f*doorSlide;
    box((Vector3){x,1.30f,-2.88f},(Vector3){1.72f,2.26f,.055f},C(157,116,80));
    box((Vector3){x,1.30f,-2.84f},(Vector3){1.55f,2.10f,.025f},C(151,111,78));
    box((Vector3){x+.77f,1.32f,-2.81f},(Vector3){.045f,.30f,.05f},C(222,190,135));
    if(secondSectionOpen&&secondDoorSlide>.88f&&!keyCollected){
        Color gold=C(226,185,73);
        DrawSphereWires((Vector3){4.12f,.78f,-3.22f},.075f,8,8,gold);
        DrawCylinderEx((Vector3){4.04f,.78f,-3.22f},(Vector3){3.74f,.78f,-3.22f},.026f,.026f,8,gold);
        box((Vector3){3.75f,.72f,-3.22f},(Vector3){.045f,.12f,.06f},gold);
        box((Vector3){3.85f,.68f,-3.22f},(Vector3){.045f,.08f,.06f},gold);
    }
    box((Vector3){2.9f,2.48f,-2.92f},(Vector3){3.5f,.07f,.2f},woodLight);
    box((Vector3){2.9f,.17f,-2.92f},(Vector3){3.5f,.07f,.2f},woodLight);
}
static void drawRoom(void)
{
    drawShell(); drawWindow(); drawDesk(); drawBedAndFrame(); drawShelf(); drawPiggy(); drawWardrobe();
    drawEscapeDoor();
    box((Vector3){4.78f,1.16f,-1.42f},(Vector3){.13f,2.34f,1.88f},woodDark);
    box((Vector3){4.69f,1.16f,-1.42f},(Vector3){.07f,2.17f,1.7f},C(164,125,91));
    DrawCylinder((Vector3){0,3.08f,0},.045f,.045f,.16f,12,woodDark);
    DrawCylinder((Vector3){0,2.98f,0},.13f,.13f,.10f,16,C(91,96,93));
    /* Four separate tapered blades keep the fan readable from each view. */
    for(int i=0;i<4;i++){
        float angle=(45.0f+i*90.0f)*DEG2RAD;
        float dx=cosf(angle),dz=sinf(angle),px=-dz,pz=dx;
        float inner=.13f,outer=.67f,rootWidth=.12f,tipWidth=.075f;
        Vector3 a={dx*inner-px*rootWidth,2.91f,dz*inner-pz*rootWidth};
        Vector3 b={dx*inner+px*rootWidth,2.91f,dz*inner+pz*rootWidth};
        Vector3 c={dx*outer+px*tipWidth,2.91f,dz*outer+pz*tipWidth};
        Vector3 d={dx*outer-px*tipWidth,2.91f,dz*outer-pz*tipWidth};
        DrawTriangle3D(a,b,c,C(132,136,129));
        DrawTriangle3D(a,c,d,C(132,136,129));
        DrawTriangle3D(c,b,a,C(132,136,129));
        DrawTriangle3D(d,c,a,C(132,136,129));
    }
    box((Vector3){0,.02f,-.15f},(Vector3){3.45f,.035f,2.18f},C(197,183,159));
}

static Camera3D makeCamera(View m)
{
    Camera3D c={0};c.up=(Vector3){0,1,0};c.fovy=55;c.projection=CAMERA_PERSPECTIVE;
    if(m==ROOM)return roomCameras[currentView];
    if(m==DESK){c.position=(Vector3){-2.55f,1.88f,2.68f};c.target=(Vector3){-4.4f,.78f,2.68f};c.fovy=53;}
    else if(m==PAPER){c.position=(Vector3){-3.48f,2.18f,2.68f};c.target=(Vector3){-4.63f,.58f,2.78f};c.fovy=42;}
    else if(m==SHELF){c.position=(Vector3){-2.15f,1.58f,-1.33f};c.target=(Vector3){-4.35f,1.35f,-1.33f};c.fovy=48;}
    else if(m==BOOK){c.position=(Vector3){-2.65f,1.72f,-.79f};c.target=(Vector3){-4.1f,1.7f,-.79f};c.fovy=30;}
    else if(m==WARDROBE){c.position=(Vector3){2.9f,1.65f,.05f};c.target=(Vector3){2.9f,1.35f,-3.0f};c.fovy=50;}
    else if(m==FRAME){c.position=(Vector3){2.6f,1.18f,1.62f};c.target=(Vector3){3.75f,1.02f,1.62f};c.fovy=35;}
    else return camera;
    return c;
}
static void beginView(View next){view=next;camera=makeCamera(next);}
static Controls controls(void)
{
    float w=(float)GetScreenWidth(),h=(float)GetScreenHeight(),s=58,g=10,x=w-(2*s+g)-24,y=h-s-30;
    Controls c={{x,y,s,s},{x+s+g,y,s,s},{24,h-76,126,48}};
    return c;
}
static void button(Rectangle r,const char *txt,const char *cap,bool active)
{
    DrawRectangleRounded(r,.18f,6,active?(Color){53,61,60,238}:(Color){95,98,92,150});DrawRectangleRoundedLines(r,.18f,6,(Color){220,209,183,255});
    DrawText(txt,(int)(r.x+(r.width-MeasureText(txt,24))*.5f),(int)r.y+2,24,RAYWHITE);
    DrawText(cap,(int)(r.x+(r.width-MeasureText(cap,10))*.5f),(int)(r.y+r.height-14),10,C(226,221,207));
}
static void statusMessage(void)
{
    if(messageTime<=0||!message[0])return;float w=(float)(MeasureText(message,20)+42);
    float x=((float)GetScreenWidth()-w)*.5f,y=(float)GetScreenHeight()-88.0f;
    DrawRectangleRounded((Rectangle){x,y,w,46.0f},.18f,6,(Color){28,34,33,235});drawTextAt(message,x+21,y+13,20,RAYWHITE);
}
static void drawUi(void)
{
    Controls c=controls();bool active=view==ROOM&&!escapeOpening;
    if(active){button(c.left,"<","PREVIOUS",true);button(c.right,">","NEXT",true);}
    if(view!=ROOM&&view!=ENDING){DrawRectangleRounded(c.back,.16f,6,(Color){53,61,60,245});DrawRectangleRoundedLines(c.back,.16f,6,(Color){220,209,183,255});DrawText("BACK",(int)c.back.x+36,(int)c.back.y+13,20,RAYWHITE);}
    if(view!=PAPER){
        DrawRectangleRounded((Rectangle){22,20,270,76},.12f,6,(Color){24,31,31,205});DrawText("ANIMATED ROOM",40,34,20,(Color){244,238,220,255});
    if(view==ROOM)DrawText(viewNames[currentView],40,63,15,(Color){195,207,195,255});
        else if(view==SHELF)DrawText("BOOKSHELF VIEW",40,63,15,(Color){195,207,195,255});
        else if(view==BOOK)DrawText("BOOK INSPECTION",40,63,15,(Color){195,207,195,255});
        else if(view==WARDROBE)DrawText(codeUnlocked?"SLIDING WARDROBE":"WARDROBE CODE",40,63,15,(Color){195,207,195,255});
        else if(view==FRAME)DrawText("FRAME INSPECTION",40,63,15,(Color){195,207,195,255});
        else if(view==DESK)DrawText("DESK VIEW",40,63,15,(Color){195,207,195,255});
    }
}
static void drawPaper(void)
{
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight();Rectangle p={sw*.23f,sh*.075f,sw*.54f,sh*.82f};
    DrawRectangleRounded((Rectangle){p.x+10,p.y+12,p.width,p.height},.015f,4,(Color){20,24,24,135});DrawRectangleRounded(p,.015f,4,(Color){246,242,229,255});DrawRectangleRoundedLines(p,.015f,4,(Color){194,181,157,255});
    float x=p.x+50;drawRectangleAt(x,p.y+43,p.width-100,2,(Color){177,157,127,255});
    drawTextAt("You are stuck inside a bedroom.",x,p.y+78,21,(Color){68,66,58,255});drawTextAt("Look around carefully and find a way out.",x,p.y+112,19,(Color){78,75,66,255});
    drawTextAt("The door is locked. The bed is not an escape plan.",x,p.y+150,17,(Color){104,96,79,255});
    drawTextAt("TRAPPED BY",x,p.y+218,18,(Color){82,65,48,255});
    const char *names[4]={"Aarya Rajendra Sane","Gargi Sachin Phadke","Akshara Mahesh Patil","Tanvi Dnyaneshwar Mhatre"};for(int i=0;i<4;i++)drawTextAt(names[i],x,p.y+251+i*31,18,(Color){71,70,62,255});
}
static void drawBookPages(void)
{
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight();Rectangle b={sw*.2f,sh*.17f,sw*.6f,sh*.66f};
    DrawRectangleRounded((Rectangle){b.x+8,b.y+12,b.width,b.height},.04f,10,(Color){28,23,18,150});DrawRectangleRounded(b,.04f,10,(Color){103,69,46,255});
    Rectangle l={b.x+12,b.y+12,b.width*.49f-14,b.height-24},r={b.x+b.width*.51f+2,b.y+12,b.width*.49f-14,b.height-24};
    DrawRectangleRounded(l,.025f,8,(Color){244,239,222,255});DrawRectangleRounded(r,.025f,8,(Color){248,243,227,255});
    drawRectangleAt(b.x+b.width*.495f,b.y+18,8,b.height-36,(Color){186,164,133,255});
    for(int i=0;i<7;i++){float yy=l.y+54+i*23;drawLineAt(l.x+24,yy,l.x+l.width-24,yy,(Color){186,176,155,255});if(i!=2&&i!=3)drawLineAt(r.x+24,yy,r.x+r.width-24,yy,(Color){186,176,155,255});}
    int size=(int)(sh*.23f);if(size<74)size=74;drawTextAt("16",r.x+r.width*.31f,r.y+r.height*.30f,size,(Color){38,39,34,255});
}
static void drawWardrobePanel(void)
{
    if(codeUnlocked)return;float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight(),w=400,x=(sw-w)*.5f,y=sh*.12f;
    DrawRectangleRounded((Rectangle){x,y,w,330},.08f,8,(Color){28,34,33,242});drawTextAt("ENTER 4-DIGIT CODE",x+30,y+20,22,RAYWHITE);
    DrawRectangleRounded((Rectangle){x+30,y+58,w-60,44},.12f,6,(Color){235,231,218,255});drawTextAt(code[0]?code:"_ _ _ _",x+46,y+67,25,(Color){36,43,42,255});
    float bw=60,bh=39,gap=8;for(int i=0;i<10;i++){int row=i/5,col=i%5;Rectangle q={x+24+col*(bw+gap),y+116+row*(bh+7),bw,bh};DrawRectangleRounded(q,.12f,5,(Color){73,83,78,255});const char *s=i==9?"0":TextFormat("%d",i+1);drawTextAt(s,q.x+23,q.y+8,21,RAYWHITE);}
    Rectangle enter={x+24,y+209,158,37},clear={x+194,y+209,158,37};DrawRectangleRounded(enter,.12f,5,(Color){112,117,79,255});DrawRectangleRounded(clear,.12f,5,(Color){95,78,68,255});
    drawTextAt("ENTER",enter.x+53,enter.y+8,18,RAYWHITE);drawTextAt("CLEAR",clear.x+54,clear.y+8,18,RAYWHITE);if(codeMessage[0])drawTextAt(codeMessage,x+24,y+263,16,RAYWHITE);
}
static void drawCarPicture(void)
{
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight(),w=sw*.58f,h=sh*.5f,x=(sw-w)*.5f,y=(sh-h)*.5f;
    DrawRectangleRounded((Rectangle){x+9,y+12,w,h},.025f,8,(Color){20,20,20,150});DrawRectangleRounded((Rectangle){x,y,w,h},.025f,8,woodDark);
    Rectangle im={x+16,y+16,w-32,h-32};DrawRectangleRec(im,(Color){150,204,218,255});
    drawCircleAt(im.x+im.width*.8f,im.y+im.height*.24f,h*.08f,(Color){246,222,162,255});
    DrawTriangle((Vector2){im.x,im.y+im.height*.64f},(Vector2){im.x+im.width*.47f,im.y+im.height*.26f},(Vector2){im.x+im.width*.68f,im.y+im.height*.64f},(Color){83,127,103,255});
    DrawTriangle((Vector2){im.x+im.width*.36f,im.y+im.height*.64f},(Vector2){im.x+im.width*.7f,im.y+im.height*.32f},(Vector2){im.x+im.width,im.y+im.height*.64f},(Color){93,142,110,255});
    drawRectangleAt(im.x,im.y+im.height*.68f,im.width,im.height*.32f,(Color){64,67,69,255});float cx=im.x+im.width*.47f,cy=im.y+im.height*.62f;
    DrawRectangleRounded((Rectangle){cx-w*.14f,cy,w*.3f,h*.12f},.35f,8,(Color){200,39,39,255});
    DrawTriangle((Vector2){cx+w*.14f,cy},(Vector2){cx+w*.21f,cy+h*.05f},(Vector2){cx+w*.14f,cy+h*.1f},(Color){200,39,39,255});
    drawRectangleAt(cx-w*.03f,cy-h*.06f,w*.09f,h*.07f,(Color){235,230,211,255});drawCircleAt(cx-w*.08f,cy+h*.09f,h*.075f,(Color){27,29,31,255});drawCircleAt(cx+w*.13f,cy+h*.09f,h*.075f,(Color){27,29,31,255});
    drawTextAt("44",cx-w*.005f,cy+h*.005f,(int)(h*.085f),RAYWHITE);
}
static void drawOverlay(void){if(view==BOOK)drawBookPages();if(view==WARDROBE)drawWardrobePanel();if(view==FRAME)drawCarPicture();}
static void submitCode(void)
{
    if(strlen(code)!=4)return;
    if(strcmp(code,"1644")==0){codeUnlocked=true;sectionOpen=true;codeMessage[0]=0;code[0]=0;snprintf(message,sizeof(message),"The first wardrobe section slides open.");messageTime=2.4f;}
    else{snprintf(codeMessage,sizeof(codeMessage),"Wrong. The clothes are still waiting.");code[0]=0;}
}
static bool wardrobeClick(Vector2 m)
{
    if(view!=WARDROBE||codeUnlocked)return false;float w=400,x=(GetScreenWidth()-w)*.5f,y=GetScreenHeight()*.12f,bw=60,bh=39,gap=8;
    for(int i=0;i<10;i++){int row=i/5,col=i%5;Rectangle q={x+24+col*(bw+gap),y+116+row*(bh+7),bw,bh};if(CheckCollisionPointRec(m,q)&&strlen(code)<4){size_t n=strlen(code);code[n]=i==9?'0':(char)('1'+i);code[n+1]=0;codeMessage[0]=0;return true;}}
    if(CheckCollisionPointRec(m,(Rectangle){x+24,y+209,158,37})){submitCode();return true;}
    if(CheckCollisionPointRec(m,(Rectangle){x+194,y+209,158,37})){code[0]=0;codeMessage[0]=0;return true;}return false;
}
static bool uiClick(Vector2 m)
{
    if(escapeOpening)return true;
    if(view==ENDING)return true;
    Controls c=controls();if(view!=ROOM&&CheckCollisionPointRec(m,c.back)){
        if(view==PAPER)beginView(paperReturn);else if(view==BOOK)beginView(SHELF);else if(view==SHELF||view==WARDROBE||view==FRAME)beginView(returnView);else beginView(ROOM);return true;}
    if(view!=ROOM)return false;
    if(CheckCollisionPointRec(m,c.left)){currentView=(currentView+3)%4;camera=roomCameras[currentView];return true;}
    if(CheckCollisionPointRec(m,c.right)){currentView=(currentView+1)%4;camera=roomCameras[currentView];return true;}return false;
}
static const HitTarget *pick(Vector2 m)
{
    Ray r=GetScreenToWorldRay(m,camera);const HitTarget *best=NULL;float nearest=1e9f;
    for(size_t i=0;i<sizeof(targets)/sizeof(targets[0]);i++){
        if(targets[i].id==CLOTHES_OBJ&&!sectionOpen)continue;
        if(targets[i].id==WARDROBE_OBJ&&view==WARDROBE&&codeUnlocked)continue;
        if(targets[i].id==KEY_OBJ&&(view!=WARDROBE||!secondSectionOpen||secondDoorSlide<=.88f||keyCollected))continue;
        RayCollision h=GetRayCollisionBox(r,targets[i].box);if(h.hit&&h.distance<nearest){nearest=h.distance;best=&targets[i];}
    }
    if(view==WARDROBE&&codeUnlocked){
        static HitTarget panels[2];
        float firstX=1.98f+1.68f*doorSlide;
        float secondX=3.78f-1.68f*secondDoorSlide;
        panels[0]=(HitTarget){FIRST_DOOR_OBJ,{{firstX-.86f,.17f,-2.92f},{firstX+.86f,2.43f,-2.78f}}};
        panels[1]=(HitTarget){SECOND_DOOR_OBJ,{{secondX-.78f,.25f,-2.95f},{secondX+.78f,2.35f,-2.87f}}};
        for(int i=0;i<2;i++){
            RayCollision h=GetRayCollisionBox(r,panels[i].box);
            if(h.hit&&h.distance<nearest){nearest=h.distance;best=&panels[i];}
        }
    }
    return best;
}
static void worldClick(Vector2 m)
{
    if(escapeOpening)return;
    if(view==ENDING||view==PAPER||view==BOOK||view==FRAME||(view==WARDROBE&&!codeUnlocked))return;
    if(view==WARDROBE){
        const HitTarget *target=pick(m);
        if(sectionOpen){
            if(target&&target->id==CLOTHES_OBJ){snprintf(message,sizeof(message),clothesClicks==0?"These don't fit.":"Still don't.");clothesClicks++;messageTime=2.2f;}
            else if(target&&target->id==FIRST_DOOR_OBJ){sectionOpen=false;snprintf(message,sizeof(message),"The first section is closed. Click the other sliding door.");messageTime=2.8f;}
            return;
        }
        if(doorSlide>.03f){snprintf(message,sizeof(message),"Close the first section before opening the other.");messageTime=2.4f;return;}
        if(secondSectionOpen){
            if(target&&target->id==KEY_OBJ){keyCollected=true;snprintf(message,sizeof(message),"You found the escape-door key.");messageTime=2.4f;return;}
            if(keyCollected&&target&&target->id==SECOND_DOOR_OBJ)secondSectionOpen=false;
            return;
        }
        if(target&&target->id==KEY_OBJ){
            keyCollected=true;snprintf(message,sizeof(message),"You found the escape-door key.");messageTime=2.4f;return;
        }
        if(!secondSectionOpen&&target&&target->id==SECOND_DOOR_OBJ){secondSectionOpen=true;snprintf(message,sizeof(message),"The second wardrobe section slides open.");messageTime=2.4f;return;}
        if(!secondSectionOpen&&target&&target->id==FIRST_DOOR_OBJ){sectionOpen=true;return;}
        return;
    }
    const HitTarget *h=pick(m);if(!h)return;
    switch(h->id){
        case PAPER_OBJ:paperReturn=view;beginView(PAPER);break;
        case SHELF_OBJ:returnView=view;beginView(SHELF);break;
        case BOOK_OBJ:beginView(BOOK);break;
        case WARDROBE_OBJ:returnView=view;code[0]=0;codeMessage[0]=0;beginView(WARDROBE);break;
        case FRAME_OBJ:returnView=view;beginView(FRAME);break;
        case CLOTHES_OBJ:if(sectionOpen){snprintf(message,sizeof(message),clothesClicks==0?"These don't fit.":"Still don't.");clothesClicks++;messageTime=2.2f;}break;
        case KEY_OBJ:if(view==WARDROBE&&secondSectionOpen&&!keyCollected){keyCollected=true;snprintf(message,sizeof(message),"You found the escape-door key.");messageTime=2.4f;}break;
        case DOOR_OBJ:
            if(keyCollected&&!escapeOpening){escapeOpening=true;escapeDoorAngle=0;messageTime=0;}
            else if(!keyCollected){snprintf(message,sizeof(message),"The escape door is locked.");messageTime=2.4f;}
            break;
        default:break;
    }
}
static void drawEnding(void)
{
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight();
    Rectangle p={sw*.22f,sh*.27f,sw*.56f,sh*.45f};
    DrawRectangleRounded(p,.04f,8,(Color){244,240,226,255});
    DrawRectangleRoundedLines(p,.04f,8,(Color){177,157,127,255});
    const char *stages[3]={"YOU ESCAPED","YOUR REWARD","Nothing!"};
    const char *text=stages[endingStage];
    drawTextAt(text,(sw-MeasureText(text,42))*.5f,p.y+126,42,(Color){54,65,58,255});
}
int main(void)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_RESIZABLE);InitWindow(1280,800,"Animated Room");if(!IsWindowReady())return 1;
    SetTargetFPS(60);SetExitKey(KEY_NULL);camera=makeCamera(view);
    while(!WindowShouldClose()){
        float dt=GetFrameTime();messageTime=fmaxf(0,messageTime-dt);
        doorSlide+=( (sectionOpen?1.0f:0.0f)-doorSlide)*fminf(dt*4,1);
        secondDoorSlide+=( (secondSectionOpen?1.0f:0.0f)-secondDoorSlide)*fminf(dt*4,1);
        if(escapeOpening){
            escapeDoorAngle=fminf(90,escapeDoorAngle+90*dt);
            if(escapeDoorAngle>=90){escapeOpening=false;view=ENDING;endingStage=0;endingStageTime=1.5f;}
        }else if(view==ENDING&&endingStage<2){
            endingStageTime-=dt;
            if(endingStageTime<=0){endingStage++;endingStageTime=1.5f;}
        }
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){Vector2 m=GetMousePosition();if(!wardrobeClick(m)&&!uiClick(m))worldClick(m);}
        if(view==WARDROBE&&!codeUnlocked){if(IsKeyPressed(KEY_BACKSPACE)&&code[0])code[strlen(code)-1]=0;if(IsKeyPressed(KEY_ENTER))submitCode();int ch=GetCharPressed();while(ch>0){size_t n=strlen(code);if(ch>='0'&&ch<='9'&&n<4){code[n]=(char)ch;code[n+1]=0;codeMessage[0]=0;}ch=GetCharPressed();}}
        BeginDrawing();ClearBackground((Color){202,211,210,255});BeginMode3D(camera);drawRoom();EndMode3D();
        if(view==PAPER)drawPaper();drawOverlay();drawUi();if(view==ENDING)drawEnding();statusMessage();EndDrawing();
    }
    CloseWindow();return 0;
}
