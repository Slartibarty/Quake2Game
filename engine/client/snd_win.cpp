#include "snd_local.h"

#include "winquake.h"
#include <dsound.h>

#define SECONDARY_BUFFER_SIZE	0x10000

// starts at 0 for disabled
static int			sample16;

static bool			dsound_init;
static DWORD		gSndBufSize;
static DWORD		locksize;

static LPDIRECTSOUND8		pDS;
static LPDIRECTSOUNDBUFFER	pDSBuf;

static const char* DSoundError( HRESULT hr )
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
==================
SNDDMA_InitDS

Direct-Sound support
Returns false if failed
==================
*/
qboolean SNDDMA_InitDS (void)
{
	HRESULT hr;

	Com_Printf("Initializing DirectSound\n");

	// Create IDirectSound using the primary sound device
//	if (DirectSoundCreate8(NULL, &pDS, NULL) != DS_OK)
	if (FAILED(CoCreateInstance(CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound8, (void**)&pDS))) 
	{
		Com_Printf("failed\n");
		SNDDMA_Shutdown();
		return false;
	}

	pDS->Initialize(NULL);

	Com_DPrintf("ok\n");

	Com_DPrintf("...setting DSSCL_PRIORITY coop level: ");

	if (FAILED(pDS->SetCooperativeLevel(cl_hwnd, DSSCL_PRIORITY))) {
		Com_Printf("failed\n");
		SNDDMA_Shutdown();
		return false;
	}
	Com_DPrintf("ok\n");

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
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	DSBUFFERDESC dsbuf{};
	dsbuf.dwSize = sizeof(dsbuf);

	// Micah: take advantage of 2D hardware if available.
	dsbuf.dwFlags = DSBCAPS_LOCHARDWARE | DSBCAPS_GETCURRENTPOSITION2;
	dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
	dsbuf.lpwfxFormat = &format;

	Com_DPrintf("...creating secondary buffer: ");
	hr = pDS->CreateSoundBuffer(&dsbuf, &pDSBuf, NULL);
//	hr = pDS->QueryInterface(IID_IDirectSoundBuffer8, (void**)&pDSBuf);
	if (FAILED(hr)) {
		// No hardware support, use software mixing
		dsbuf.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_GETCURRENTPOSITION2;
		hr = pDS->CreateSoundBuffer(&dsbuf, &pDSBuf, NULL);
		if (FAILED(hr)) {
			Com_Printf("failed to create secondary buffer - %s\n", DSoundError(hr));
			SNDDMA_Shutdown();
			return false;
		}
		Com_DPrintf("forced to software.  ok\n");
	}
	else {
		Com_Printf("locked hardware.  ok\n");
	}

	// Make sure mixer is active
	if (FAILED(pDSBuf->Play(0, 0, DSBPLAY_LOOPING))) {
		Com_Printf("*** Looped sound play failed ***\n");
		SNDDMA_Shutdown();
		return false;
	}

	DSBCAPS dsbcaps{};
	dsbcaps.dwSize = sizeof(dsbcaps);
	// get the returned buffer size
	if (FAILED(pDSBuf->GetCaps(&dsbcaps))) {
		Com_Printf("*** GetCaps failed ***\n");
		SNDDMA_Shutdown();
		return false;
	}

	gSndBufSize = dsbcaps.dwBufferBytes;

	dma.channels = format.nChannels;
	dma.samplebits = format.wBitsPerSample;
	dma.speed = format.nSamplesPerSec;
	dma.samples = gSndBufSize / (dma.samplebits / 8);
	dma.submission_chunk = 1;
	dma.buffer = NULL;			// must be locked first

	sample16 = (dma.samplebits / 8) - 1;

	SNDDMA_BeginPainting();
	if (dma.buffer)
		memset(dma.buffer, 0, dma.samples * dma.samplebits / 8);
	SNDDMA_Submit();
	return 1;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos(void)
{
	DWORD	mmtime;
	int		s;
	DWORD	dwWrite;

	if (!dsound_init) {
		return 0;
	}

	pDSBuf->GetCurrentPosition(&mmtime, &dwWrite);

	s = mmtime;

	s >>= sample16;

	s &= (dma.samples - 1);

	return s;
}

/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
void SNDDMA_BeginPainting (void)
{
	int		reps;
	DWORD	dwSize2;
	DWORD	*pbuf, *pbuf2;
	HRESULT	hresult;
	DWORD	dwStatus;

	if ( !pDSBuf ) {
		return;
	}

	// if the buffer was lost or stopped, restore it and/or restart it
	if ( pDSBuf->GetStatus (&dwStatus) != DS_OK ) {
		Com_Printf ("Couldn't get sound buffer status\n");
	}
	
	if (dwStatus & DSBSTATUS_BUFFERLOST)
		pDSBuf->Restore ();
	
	if (!(dwStatus & DSBSTATUS_PLAYING))
		pDSBuf->Play(0, 0, DSBPLAY_LOOPING);

	// lock the dsound buffer

	reps = 0;
	dma.buffer = NULL;

	while ((hresult = pDSBuf->Lock(0, gSndBufSize, (void **)&pbuf, &locksize, 
								   (void **)&pbuf2, &dwSize2, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			Com_Printf( "SNDDMA_BeginPainting: Lock failed with error '%s'\n", DSoundError( hresult ) );
			S_Shutdown ();
			return;
		}
		else
		{
			pDSBuf->Restore( );
		}

		if (++reps > 2)
			return;
	}
	dma.buffer = (unsigned char *)pbuf;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void SNDDMA_Submit(void)
{
	// unlock the dsound buffer
	if (pDSBuf) {
		pDSBuf->Unlock(dma.buffer, locksize, NULL, 0);
	}
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/
qboolean SNDDMA_Init(void)
{
	memset((void*)&dma, 0, sizeof(dma));
	dsound_init = false;

	// Init COM
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	/* Init DirectSound */
	if (!SNDDMA_InitDS()) {
		CoUninitialize();
		return false;
	}

	dsound_init = true;

	Com_DPrintf("Completed successfully\n");

	return true;
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown(void)
{
	Com_DPrintf("Shutting down sound system\n");

	if (dsound_init)
	{
		Com_DPrintf("Destroying DS buffers\n");
		if (pDS)
		{
			Com_DPrintf("...setting NORMAL coop level\n");
			pDS->SetCooperativeLevel(cl_hwnd, DSSCL_NORMAL);
		}

		if (pDSBuf)
		{
			Com_DPrintf("...stopping and releasing sound buffer\n");
			pDSBuf->Stop();
			pDSBuf->Release();
		}

		pDSBuf = NULL;

		dma.buffer = NULL;

		Com_DPrintf("...releasing DS object\n");
		pDS->Release();

		CoUninitialize();
	}

	pDS = NULL;
	pDSBuf = NULL;
	dsound_init = false;
	memset((void*)&dma, 0, sizeof(dma));
}

/*
===========
SNDDMA_Activate

Called when the main window gains or loses focus.
The window have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void SNDDMA_Activate (qboolean active)
{
	// SlartTodo: What?
	if (dsound_init) {
		if (DS_OK != pDS->SetCooperativeLevel(cl_hwnd, DSSCL_PRIORITY)) {
			Com_Printf("sound SetCooperativeLevel failed\n");
			SNDDMA_Shutdown();
		}
	}
}
