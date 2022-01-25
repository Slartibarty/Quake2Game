
#include "cl_local.h"

static constexpr float pm_maxspeed = 320;
static constexpr float pm_accelerate = 10;
static constexpr float pm_noclipfriction = 12;

struct Camera
{
	DirectX::XMFLOAT3 origin;
	DirectX::XMFLOAT3 angles;
};

static struct devDefs_t
{
	Camera camera;
} dev;

struct userCmd_t
{
	float forwardMove, leftMove, upMove;
};

static vec3_t s_velocity;
static userCmd_t s_cmd;

/*
===================
PM_Accelerate

Handles user intended acceleration
===================
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel )
{
	float addspeed, accelspeed, currentspeed;

	// See if we are changing direction a bit
	currentspeed = DotProduct( s_velocity, wishdir );

	// Reduce wishspeed by the amount of veer
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done
	if ( addspeed <= 0.0f ) {
		return;
	}

	// Determine amount of accleration
	accelspeed = accel * com_deltaTime * wishspeed;

	// Cap at addspeed
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}

	// Adjust velocity
	for ( int i = 0; i < 3; ++i )
	{
		s_velocity[i] += accelspeed * wishdir[i];
	}
}

/*
===================
PM_NoclipMove
===================
*/
static void PM_NoclipMove( userCmd_t &cmd )
{
	int			i;
	float		speed, drop, friction, newspeed, stopspeed;
	float		wishspeed;
	vec3_t		wishdir;

	vec3_t forward, right, up;
	AngleVectors( (float *)&dev.camera.angles, forward, right, up );

	// friction
	speed = VectorLength( s_velocity );
	if ( speed < 20.0f ) {
		VectorClear( s_velocity );
	}
	else {
		stopspeed = pm_maxspeed * 0.3f;
		if ( speed < stopspeed ) {
			speed = stopspeed;
		}
		friction = pm_noclipfriction;
		drop = speed * friction * com_deltaTime;

		// scale the velocity
		newspeed = speed - drop;
		if ( newspeed < 0 ) {
			newspeed = 0;
		}

		VectorScale( s_velocity, newspeed / speed, s_velocity );
	}

	// accelerate

	for ( i = 0; i < 3; ++i ) {
		wishdir[i] = forward[i] * cmd.forwardMove + right[i] * cmd.leftMove;
	}
	wishdir[2] += cmd.upMove;

	wishspeed = VectorNormalize( wishdir );

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	dev.camera.origin.x += com_deltaTime * s_velocity[0];
	dev.camera.origin.y += com_deltaTime * s_velocity[1];
	dev.camera.origin.z += com_deltaTime * s_velocity[2];
}

static void CL_ReportCamera()
{
	Com_Printf( "Origin: %.6f %.6f %.6f\n", dev.camera.origin.x, dev.camera.origin.y, dev.camera.origin.z );
	Com_Printf( "Angles: %.6f %.6f %.6f\n", dev.camera.angles.x, dev.camera.angles.y, dev.camera.angles.z );
}

DirectX::XMMATRIX CL_BuildViewMatrix()
{
	using namespace DirectX;

	vec3 viewOrg = std::bit_cast<vec3>( dev.camera.origin );
	vec3 viewAng = std::bit_cast<vec3>( dev.camera.angles );

	// Invert pitch
	viewAng[PITCH] = -viewAng[PITCH];

	vec3 forward;
	forward.x = cos( DEG2RAD( viewAng[YAW] ) ) * cos( DEG2RAD( viewAng[PITCH] ) );
	forward.y = sin( DEG2RAD( viewAng[YAW] ) ) * cos( DEG2RAD( viewAng[PITCH] ) );
	forward.z = sin( DEG2RAD( viewAng[PITCH] ) );
	forward.NormalizeFast();

	XMVECTOR eyePosition = XMLoadFloat3( (XMFLOAT3 *)&viewOrg );
	XMVECTOR eyeDirection = XMLoadFloat3( (XMFLOAT3 *)&forward );
	XMVECTOR upDirection = XMVectorSet( 0.0f, 0.0f, 1.0f, 0.0f );

	return XMMatrixLookToRH( eyePosition, eyeDirection, upDirection );
}

void CL_DevFrame()
{
	s_cmd.forwardMove += pm_maxspeed * Input_GetKeyState( 'W' );
	s_cmd.forwardMove -= pm_maxspeed * Input_GetKeyState( 'S');

	s_cmd.leftMove += pm_maxspeed * Input_GetKeyState( 'D' );
	s_cmd.leftMove -= pm_maxspeed * Input_GetKeyState( 'A' );

	s_cmd.upMove += pm_maxspeed * Input_GetKeyState( 'E' );
	s_cmd.upMove -= pm_maxspeed * Input_GetKeyState( 'Q' );

	PM_NoclipMove( s_cmd );
	memset( &s_cmd, 0, sizeof( s_cmd ) );

	//CL_ReportCamera();
}
