#include "snd_local.h"

#define DIRECTSOUND_VERSION 0x1000

#include <dsound.h>
#include "winquake.h"

#define SECONDARY_BUFFER_SIZE 0x10000

// starts at 0 for disabled
static int			sample16;

static bool			dsound_init;
static DWORD		gSndBufSize;
static DWORD		locksize;

static LPDIRECTSOUND8		pDS;
static LPDIRECTSOUNDBUFFER	pDSBuf;

static const char *DSoundError( HRESULT hr )
{
	switch ( hr )
	{
	case DSERR_BUFFERLOST:
		return "DSERR_BUFFERLOST";
	case DSERR_INVALIDCALL:
		return "DSERR_INVALIDCALLS";
	case DSERR_INVALIDPARAM:
		return "DSERR_INVALIDPARAM";
	case DSERR_PRIOLEVELNEEDED:
		return "DSERR_PRIOLEVELNEEDED";
	}

	return "unknown";
}

/*
========================
SNDDMA_InitDS

Direct-Sound support
Returns false if failed
========================
*/
static bool SNDDMA_InitDS()
{
	HRESULT hr;

	Com_Print( "Initializing DirectSound\n" );

	// Create IDirectSound using the primary sound device
	//if (DirectSoundCreate8(NULL, &pDS, NULL) != DS_OK)
	if ( FAILED( CoCreateInstance( CLSID_DirectSound8, nullptr, CLSCTX_INPROC_SERVER, IID_IDirectSound8, (void **)&pDS ) ) )
	{
		Com_Print( "failed\n" );
		SNDDMA_Shutdown();
		return false;
	}

	pDS->Initialize( nullptr );

	Com_DPrint( "ok\n" );

	Com_DPrint( "...setting DSSCL_PRIORITY coop level: " );

	if ( FAILED( pDS->SetCooperativeLevel( cl_hwnd, DSSCL_PRIORITY ) ) )
	{
		Com_Print( "failed\n" );
		SNDDMA_Shutdown();
		return false;
	}
	Com_DPrint( "ok\n" );

	// create the secondary buffer we'll actually work with
	dma.channels = 2;
	dma.samplebits = 16;
	dma.speed = 44100;

	WAVEFORMATEX format{};
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.samplebits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	DSBUFFERDESC bufDesc{};
	bufDesc.dwSize = sizeof( bufDesc );
	// Micah: take advantage of 2D hardware if available
	bufDesc.dwFlags = DSBCAPS_LOCHARDWARE | DSBCAPS_GETCURRENTPOSITION2;
	bufDesc.dwBufferBytes = SECONDARY_BUFFER_SIZE;
	bufDesc.lpwfxFormat = &format;

	Com_DPrint( "...creating secondary buffer: " );

	hr = pDS->CreateSoundBuffer( &bufDesc, &pDSBuf, nullptr );
	if ( FAILED( hr ) )
	{
		// No hardware support, use software mixing
		bufDesc.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_GETCURRENTPOSITION2;
		hr = pDS->CreateSoundBuffer( &bufDesc, &pDSBuf, nullptr );
		if ( FAILED( hr ) )
		{
			Com_Printf( "failed to create secondary buffer - %s\n", DSoundError( hr ) );
			SNDDMA_Shutdown();
			return false;
		}
		Com_DPrint( "forced to software.  ok\n" );
	}
	else
	{
		Com_Print( "locked hardware.  ok\n" );
	}

	// Make sure mixer is active
	if ( FAILED( pDSBuf->Play( 0, 0, DSBPLAY_LOOPING ) ) )
	{
		Com_Print( "*** Looped sound play failed ***\n" );
		SNDDMA_Shutdown();
		return false;
	}

	DSBCAPS dsbcaps{};
	dsbcaps.dwSize = sizeof( dsbcaps );
	// get the returned buffer size
	if ( FAILED( pDSBuf->GetCaps( &dsbcaps ) ) )
	{
		Com_Print( "*** GetCaps failed ***\n" );
		SNDDMA_Shutdown();
		return false;
	}

	gSndBufSize = dsbcaps.dwBufferBytes;

	dma.channels = format.nChannels;
	dma.samplebits = format.wBitsPerSample;
	dma.speed = format.nSamplesPerSec;
	dma.samples = gSndBufSize / (dma.samplebits / 8);
	dma.submission_chunk = 1;
	dma.buffer = nullptr;		// must be locked first

	sample16 = (dma.samplebits / 8) - 1;

	SNDDMA_BeginPainting();
	if ( dma.buffer ) {
		memset( dma.buffer, 0, dma.samples * dma.samplebits / 8 );
	}
	SNDDMA_Submit();

	return true;
}

/*
========================
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
========================
*/
int SNDDMA_GetDMAPos()
{
	DWORD	dwTime;
	int		s;

	if ( !pDSBuf ) {
		return 0;
	}

	pDSBuf->GetCurrentPosition( &dwTime, nullptr );

	s = dwTime;

	s >>= sample16;

	s &= ( dma.samples - 1 );

	return s;
}

/*
========================
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
========================
*/
void SNDDMA_BeginPainting()
{
	if ( !pDSBuf ) {
		return;
	}

	DWORD dwStatus;

	// if the buffer was lost or stopped, restore it and/or restart it
	if ( pDSBuf->GetStatus( &dwStatus ) != DS_OK ) {
		Com_Print( "Couldn't get sound buffer status\n" );
	}

	if ( dwStatus & DSBSTATUS_BUFFERLOST ) {
		pDSBuf->Restore();
	}

	if ( !( dwStatus & DSBSTATUS_PLAYING ) ) {
		pDSBuf->Play( 0, 0, DSBPLAY_LOOPING );
	}

	// lock the dsound buffer

	HRESULT hresult;
	DWORD dwSize2;
	void *pBuf, *pBuf2;

	int reps = 0;
	dma.buffer = nullptr;

	while ( ( hresult = pDSBuf->Lock( 0, gSndBufSize, &pBuf, &locksize, &pBuf2, &dwSize2, 0 ) ) != DS_OK )
	{
		if ( hresult != DSERR_BUFFERLOST )
		{
			Com_Printf( "SNDDMA_BeginPainting: Lock failed with error '%s'\n", DSoundError( hresult ) );
			S_Shutdown();
			return;
		}

		pDSBuf->Restore();

		if ( ++reps > 2 ) {
			return;
		}
	}

	dma.buffer = (unsigned char *)pBuf;
}

/*
========================
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
========================
*/
void SNDDMA_Submit()
{
	// unlock the dsound buffer
	if ( pDSBuf )
	{
		pDSBuf->Unlock( dma.buffer, locksize, nullptr, 0 );
	}
}

/*
========================
SNDDMA_Init
========================
*/
bool SNDDMA_Init()
{
	memset( &dma, 0, sizeof( dma ) );
	dsound_init = false;

	if ( !SNDDMA_InitDS() )
	{
		CoUninitialize();
		return false;
	}

	dsound_init = true;

	Com_DPrint( "Completed successfully\n" );

	return true;
}

/*
========================
SNDDMA_Shutdown
========================
*/
void SNDDMA_Shutdown()
{
	Com_DPrint( "Shutting down sound system\n" );

	if ( dsound_init )
	{
		Com_DPrint( "Destroying DS buffers\n" );
		if ( pDS )
		{
			Com_DPrint( "...setting NORMAL coop level\n" );
			pDS->SetCooperativeLevel( cl_hwnd, DSSCL_NORMAL );
		}

		if ( pDSBuf )
		{
			Com_DPrint( "...stopping and releasing sound buffer\n" );
			pDSBuf->Stop();
			pDSBuf->Release();
		}

		dma.buffer = nullptr;

		Com_DPrint( "...releasing DS object\n" );
		pDS->Release();
	}

	pDS = nullptr;
	pDSBuf = nullptr;
	dsound_init = false;
	memset( &dma, 0, sizeof( dma ) );
}

/*
========================
SNDDMA_Activate
========================
*/
void SNDDMA_Activate( bool active )
{
	// SlartTodo: What?
	if ( pDS )
	{
		if ( pDS->SetCooperativeLevel( cl_hwnd, DSSCL_PRIORITY ) != DS_OK )
		{
			Com_Print( "DSound SetCooperativeLevel failed\n" );
			SNDDMA_Shutdown();
		}
	}
}
