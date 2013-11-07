#include "Engine.h"

#include <stdio.h>
#include <time.h>

#include "FlowUI.h"
#include "FlowFlinger.h"
#include "CurlWrap.h"
#include "Globals.h"

#include "ImageLoadTask.h"

#include "GalleryTable.h"
#include "Page_Gallery.h"
#include "Page_Game.h"
#include "Page_About.h"
#include "Page_MainMenu.h"

#include "Game.h"

static int TileColors[] = { 0xFFFFFF, 0xFF00FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0x00FFFF, 0xAA00AA,
                            0xAA0000, 0x00AA00, 0x0000AA, 0xAAAA00, 0x00AAAA, 0xCCAACC, 0xAACCAA, 0xCCCCAA
                          };

LVector2 g_Pos;

clPuzzle g_Game;

clPtr<clGallery> g_Gallery;

bool g_InGallery;

clAudioThread g_Audio;
clPtr<clCanvas> g_Canvas;

clPtr<clWorkerThread> g_Loader;

sLGLAPI* LGL3 = NULL;

clPtr<clFlowUI> g_Flow;

clPtr<clTextRenderer> g_TextRenderer;
int g_Font;

clPtr<clGLSLShaderProgram> BackShader;
clPtr<clGLTexture> g_Texture;
clPtr<clGUI> g_GUI;

clPtr<clGUIPage> Page_MainMenu;
clPtr<clGUIPage> Page_Game;
clPtr<clGUIPage> Page_About;
clPtr<clGUIPage> Page_Gallery;

clPtr<clFileSystem> g_FS;

vec2   g_MousePos;
double g_MouseTime = 0.0;

vec2 Env_GetMouse()
{
	return g_MousePos;
}

double Env_GetMouseTime()
{
	return g_MouseTime;
}

double Env_GetSeconds()
{
	return ( double )clock() / 1000.0;
}

void DrawTile( clPuzzle* g, int i, int j, const vec4& Color )
{
	if ( i < 0 || j < 0 || i >= g->FColumns || j >= g->FRows ) { return; }

	clTile* Tile = g->GetTile( i, j );
	Tile->SetTarget( i, j );

	float X = Tile->FCur.x, Y = Tile->FCur.y;
	float TW = 1.0f / g->FColumns, TH = 1.0f / g->FRows;

	vec4 TilePosition( TW * ( X + 0 ), TH * ( Y + 0 ), TW * ( X + 1 ), TH * ( Y + 1 ) );

	const LRect* ClipRect = Tile->GetRect();

	g_Canvas->TexturedRect2DClipped( TilePosition.x, TilePosition.y, TilePosition.z, TilePosition.w, LVector4( 1 ), g_Texture, ClipRect->ToVector4() );

//void clCanvas::TexturedRect2DTiled( float X1, float Y1, float X2, float Y2, int TilesX, int TilesY, const LVector4& Color, const clPtr<clGLTexture>& Texture )
//	Canvas->TexturedRectTiled( TilePosition.x, TilePosition.y, TilePosition.z, TilePosition.w, 1.0f, 1.0f, g_Texture,
//		Color, &Tile->FRect );
}

vec4 UIntToColor( int a )
{
	float r = ( float )( a & 0xFF ) / 255.0;
	float g = ( float )( ( a >>  8 ) & 0xFF ) / 255.0;
	float b = ( float )( ( a >> 16 ) & 0xFF ) / 255.0;

	return vec4( r, g, b, 1.0f );
}

void RenderGame( clPuzzle* g )
{
	LGL3->glClearColor( 0.0, 0.0, 0.0, 0.0 );
	LGL3->glClear( GL_COLOR_BUFFER_BIT );

	// draw selected tile (if any)
	if ( g->FMovingImage && g->FClickedI >= 0 && g->FClickedJ >= 0 && g->FClickedI < g->FColumns && g->FClickedJ < g->FRows )
	{
		vec2 MCI = Env_GetMouse();
		int NewI = g->FClickedI, NewJ = g->FClickedJ;
		float PosX = Linderdaum::Math::Clamp( MCI.x + g->FOfsX, 0.0f, 1.0f ) * g->FColumns;
		float PosY = Linderdaum::Math::Clamp( MCI.y + g->FOfsY, 0.0f, 1.0f ) * g->FRows;
		g->GetTile( NewI, NewJ )->MoveTo( PosX, PosY );
	}

	// draw static game field
	for ( int i = 0; i != g->FColumns; i++ )
		for ( int j = 0; j != g->FRows; j++ )
			if ( g->FClickedI != i || g->FClickedJ != j )
			{
				clTile* T = g->GetTile( i, j );
				DrawTile( g, i, j, UIntToColor( TileColors[T->FOriginY * g->FColumns + T->FOriginX] ) );
			}

	// draw moving tile
	if ( g->FClickedI >= 0 && g->FClickedJ >= 0 )
	{
		clTile* T = g->GetTile( g->FClickedI, g->FClickedJ );
		DrawTile( g, g->FClickedI, g->FClickedJ, UIntToColor( TileColors[T->FOriginY * g->FColumns + T->FOriginX] ) );
	}
}

class clMyFlowFlinger: public clFlowFlinger
{
public:
	clMyFlowFlinger() {}
	virtual ~clMyFlowFlinger() {}

	virtual bool HandleSelection( float mx, float my )
	{
		int Index = g_Flow->GetImageUnderCursor( mx, 0.5f );

		if ( Index > -1 )
		{
			// switch to game
			clPtr<sImageDescriptor> Img = g_Gallery->GetImage( Index );

			if ( Img.GetInternalPtr() )
			{
				g_Texture = Img->FTexture;
			}

			g_GUI->SetActivePage( Page_Game );
		}

		return false;
	}
};

class clResponder: public iGestureResponder
{
	virtual void Event_PointerChanged( int PointerID, const LVector2& Pnt, bool Pressed )
	{
		// to GalleryPage::OnTouch ?
		g_MousePos = Pnt;
		g_MouseTime = Env_GetSeconds();

		if ( g_InGallery )
		{
			if ( Pressed )
			{
				g_Flow->FFlinger->OnTouch( true );
			}
			else
			{
				g_Flow->FFlinger->OnTouch( false );
			}
		}

		// new
		if ( PointerID != 0 ) { return; }

		g_Pos = Pnt;
		g_GUI->OnTouch( Pnt, Pressed );
	}

	// new
	virtual void Event_UpdateGesture( const sMotionData& Data )
	{
		if ( Data.GetNumTouchPoints() != 1 )
		{
			g_GUI->OnTouch( g_Pos, false );
		}
	}

	virtual void Event_PointerMoved( int PointerID, const LVector2& Pnt )
	{
		g_MousePos = Pnt;
		g_MouseTime = Env_GetSeconds();

		// new
		if ( PointerID == 0 ) { g_Pos = Pnt; }

		g_GUI->OnTouch( Pnt, true );
	}
} Responder;


clPtr<clGLTexture> LoadTexture( const std::string& FileName )
{
	clPtr<clBitmap> Bmp = clBitmap::LoadImg( g_FS->CreateReader( FileName ) );
	clPtr<clGLTexture> Texture = new clGLTexture();
	Texture->LoadFromBitmap( Bmp );
	return Texture;
}

void InitGUI()
{
	////
	g_GUI = new clGUI();

	Page_MainMenu = new clPage_MainMenu;
	Page_Game     = new clPage_Game;
	Page_About    = new clPage_About;
	Page_Gallery  = new clPage_Gallery;

	Page_Gallery->FFallbackPage = Page_MainMenu;
	Page_Game->FFallbackPage = Page_MainMenu;
	Page_About->FFallbackPage = Page_MainMenu;

	g_GUI->AddPage( Page_MainMenu );
	g_GUI->AddPage( Page_Gallery );
	g_GUI->AddPage( Page_Game );
	g_GUI->AddPage( Page_About );

	Page_MainMenu->AddButton( new clGUIButton( LRect( 0.3f, 0.1f, 0.7f, 0.3f ), "New Game", Page_Gallery  ) );
	Page_MainMenu->AddButton( new clGUIButton( LRect( 0.3f, 0.4f, 0.7f, 0.6f ), "About",    Page_About ) );
	Page_MainMenu->AddButton( new clGUIButton( LRect( 0.3f, 0.7f, 0.7f, 0.9f ), "Exit",     NULL       ) );

	g_GUI->SetActivePage( Page_MainMenu );
	////

	g_TextRenderer = new clTextRenderer();
	g_Font = g_TextRenderer->GetFontHandle( "default.ttf" );
}

void OnStart( const std::string& RootPath )
{
	g_FS = new clFileSystem();
	g_FS->Mount( "." );
#if defined(ANDROID)
	g_FS->Mount( RootPath );
	g_FS->AddAliasMountPoint( RootPath, "assets" );
#endif

	g_Canvas = new clCanvas();

//	Texture = LoadTexture( "NoImageAvailable.png" );

	g_Texture = LoadTexture( "NoImageAvailable.png" );

	g_Flow = new clFlowUI( new clMyFlowFlinger(), 15 );

	g_Responder = &Responder;

	g_Loader = new clWorkerThread();
	g_Loader->Start( iThread::Priority_Normal );

	// init gui
	InitGUI();

	// init audio
	LoadOGG();
	LoadModPlug();

	g_Audio.Start( iThread::Priority_Normal );
	g_Audio.Wait();

	// Initialize the network and events queue
	Curl_Load();

	g_Events = new iAsyncQueue();

	g_Downloader = new clDownloader();
	g_Downloader->FEventQueue = g_Events.GetInternalPtr();

	// Start the Picasa list download
	g_Gallery = new clGallery();
	g_Gallery->StartListDownload();
}

void OnDrawFrame()
{
//	RenderDirect(g_Flow);
	g_GUI->Render();
}

void OnTimer( float Delta )
{
	// dispatch events here
	g_Events->DemultiplexEvents();

	g_GUI->Update( Delta );
}

void OnKey( int code, bool pressed )
{
	g_GUI->OnKey( g_Pos, code, pressed );

	if ( !pressed )
	{
//		ProcessClick( false );
		return;
	}
}

void OnMouseDown( int btn, const LVector2& Pos )
{
	g_Pos = Pos;

	g_GUI->OnTouch( Pos, true );

	if ( g_InGallery )
	{
		g_MousePos = Pos;
		g_MouseTime = Env_GetSeconds();

		g_Flow->FFlinger->OnTouch( true );
	}
}

void OnMouseMove( const LVector2& Pos )
{
	g_MousePos = Pos;
	g_Pos = Pos;
	g_MouseTime = Env_GetSeconds();
}

void OnMouseUp( int btn, const LVector2& Pos )
{
	g_Pos = Pos;
	g_GUI->OnTouch( Pos, false );

	if ( g_InGallery )
	{
		g_MousePos = Pos;
		g_MouseTime = Env_GetSeconds();

		g_Flow->FFlinger->OnTouch( false );
	}
}

void OnStop()
{
}
