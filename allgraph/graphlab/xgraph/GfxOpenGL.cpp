#ifdef _WINDOWS
/* MSVC++ 6 must have <windows.h> included before OpenGL headers or
   else compile errors will result.  */
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#include "GfxOpenGL.h"

GfxOpenGL::GfxOpenGL()
{
}

GfxOpenGL::~GfxOpenGL()
{
	Shutdown();
}

bool GfxOpenGL::Init()
{
	m_viewScale = 1.0f;
	m_xRotateState = 0.0f;
	m_yRotateState = 0.0f;
	m_quadSpeed = false;
	glLoadIdentity();
	glGetFloatv(GL_MODELVIEW_MATRIX, rotateMatrix);
	//m_eyeAngle.x = 0.0f;
	//m_eyeAngle.y = 0.0f;

	glClearColor(0.0f, 0.7f, 1.0f, 1.0f);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_NORMALIZE); // Otherwise scaling scales normals too.
	glShadeModel(GL_FLAT);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	GLfloat v[] = { 0.0f, 0.7071068f, 0.7071068f, 0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, v);
	v[0] = 0.2f; v[1] = 0.2f;
	v[2] = 0.2f; v[3] = 1.0f;
	glLightfv(GL_LIGHT0, GL_AMBIENT, v);
	v[0] = 0.8f; v[1] = 0.8f;
	v[2] = 0.8f; v[3] = 1.0f;
	glLightfv(GL_LIGHT0, GL_DIFFUSE, v);
	glPointSize((GLfloat)10.0f);
	return true;
}

void GfxOpenGL::SetupProjection(unsigned width, unsigned height)
{
	if (height == 0)					//don't want a divide by zero
	{
		height = 1;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, width, height);		//reset the viewport to new dimensions
	glMatrixMode(GL_PROJECTION);			//set projection matrix current matrix
	glLoadIdentity();						//reset projection matrix

	//calculate aspect ratio of window
	gluPerspective(20.0f, (GLfloat)width / (GLfloat)height, 16.0f, 64.0f);

	glMatrixMode(GL_MODELVIEW);				//set modelview matrix
	glLoadIdentity();						//reset modelview matrix

	m_windowWidth = width;
	m_windowHeight = height;
}

bool GfxOpenGL::Shutdown()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHT0);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	return true;
}


void GfxOpenGL::Prepare(float dt)
{
	angle m_eyeAngle;
	m_eyeAngle.x = m_xRotateState * 360.0f * dt;
	m_eyeAngle.y = m_yRotateState * 360.0f * dt;
	if (m_eyeAngle.x == 0 && m_eyeAngle.y == 0)
		return;
	if (m_quadSpeed == true)
	{
		m_eyeAngle.x *= 4;
		m_eyeAngle.y *= 4;
	}
	glLoadIdentity();
	glRotatef(m_eyeAngle.y, 0.0f, 1.0f, 0.0f);
	glRotatef(m_eyeAngle.x, 1.0f, 0.0f, 0.0f);
	glMultMatrixf(rotateMatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, rotateMatrix);
}

void GfxOpenGL::Render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -32.0f);
	/*glRotatef(m_eyeAngle.x, 1.0f, 0.0f, 0.0f);
	glRotatef(m_eyeAngle.y, 0.0f, 1.0f, 0.0f);*/
	glMultMatrixf(rotateMatrix);
	glScalef(m_viewScale, m_viewScale, m_viewScale);

	/*glBegin(GL_TRIANGLE_STRIP);
	glColor3f(1.0f, 1.0f, 0.0f); glVertex3f(-1.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
	glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(1.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 0.5f, 0.0f); glVertex3f(1.0f, -1.0f, 0.0f);
	glColor3f(1.0f, 1.0f, 0.0f); glVertex3f(2.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(2.0f, -1.0f, 0.0f);
	glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(3.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 0.5f, 0.0f); glVertex3f(3.0f, -1.0f, 0.0f);
	glEnd();*/

	glBegin(GL_QUADS);
	glColor3f(0.75f, 0.75f, 0.95f);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex3f(-1.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f, -1.0f, 0.0f);
	glVertex3f(1.0f, -1.0f, 0.0f);
	glVertex3f(1.0f, 1.0f, 0.0f);

	glColor3f(0.95f, 0.75f, 0.75f);
	glNormal3f(0.707f, 0.0f, 0.707f);
	glVertex3f(-0.707f, 1.0f, 0.25f);
	glVertex3f(-0.707f, -1.0f, 0.25f);
	glVertex3f(0.707f, -1.0f, -1.164f);
	glVertex3f(0.707f, 1.0f, -1.164f);

	glColor3f(0.5f, 0.95f, 0.5f);
	glNormal3f(0.0f, -1.0f, 0.0f);
	glVertex3f(-1.0f, 0.875f, 1.0f);
	glVertex3f(-1.0f, 0.875f, -1.0f);
	glVertex3f(1.0f, 0.875f, -1.0f);
	glVertex3f(1.0f, 0.875f, 1.0f);
	glEnd();

	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(-0.5f, 0.0f, 1.0f);
	glVertex3f(0.5f, 0.0f, -0.25f);
	glEnd();
	glBegin(GL_POINTS);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.375f);
	glEnd();
	glEnable(GL_LIGHTING);
}

void GfxOpenGL::MouseRotate(int xScr, int yScr, bool firstTime)
{
	float x = ConvScrCoordsX(xScr);
	float y = ConvScrCoordsY(yScr);
	static float oldX, oldY, oldZ;

	if (firstTime == true)
	{
		oldX = x;
		oldY = y;
		oldZ = RAD2DEG(atan2f(y, x));
	}
	else
	{
		float dx = x - oldX;
		float dy = y - oldY;
		oldX = x;
		oldY = y;
		float rotX = dy * -180;
		float rotY = dx * 180;
		float rotZ = RAD2DEG(atan2f(y, x));
		if (rotZ > 0.0f && oldZ < 0.0f || rotZ < 0.0f && oldZ > 0.0f)
			rotZ += oldZ;
		else
			rotZ -= oldZ;
		oldZ = RAD2DEG(atan2f(y, x));
		float blend = sqrtf(x * x + y * y);
		rotX = rotX * MAX((1.0f - blend), 0.0f);
		rotY = rotY * MAX((1.0f - blend), 0.0f);
		rotZ *= blend;
		glLoadIdentity();
		glRotatef(rotZ, 0.0f, 0.0f, 1.0f);
		glRotatef(rotY, 0.0f, 1.0f, 0.0f);
		glRotatef(rotX, 1.0f, 0.0f, 0.0f);
		glMultMatrixf(rotateMatrix);
		glGetFloatv(GL_MODELVIEW_MATRIX, rotateMatrix);
	}
}

void GfxOpenGL::Get3DPick(int x, int y, unsigned& pickedFace, Point& finalPick)
{
	//Point finalPick;
	GLdouble modelMat[16], projMat[16];
	GLint viewport[4];
	GLdouble nVec[3], fVec[3], dir[3];
	float tPts[6][3];
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -32.0f);
	glMultMatrixf(rotateMatrix);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
	glPopMatrix();
	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluUnProject((GLdouble)x, (GLdouble)viewport[3] - (GLdouble)y, (GLdouble)0.0, modelMat, projMat, viewport, &nVec[0], &nVec[1], &nVec[2]);
	gluUnProject((GLdouble)x, (GLdouble)viewport[3] - (GLdouble)y, (GLdouble)1.0, modelMat, projMat, viewport, &fVec[0], &fVec[1], &fVec[2]);
	dir[0] = fVec[0] - nVec[0]; dir[1] = fVec[1] - nVec[1]; dir[2] = fVec[2] - nVec[2];
	//The rest of this function ought to be done in a better way...
	//Perhaps this data should be clused with the rest of the RubiksCube class.
	bool checklist[6] = { true, true, true, true, true, true };
	float pNorms[6][3] = { {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f},
						   {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f} };
	float ofst[6] = { -3.0f, -3.0f, -3.0f, -3.0f, -3.0f, -3.0f };

	//PLANE CLIPPING!
	GLdouble clipVec[3] = { 0.0, 0.0, 0.0 };
	unsigned  i;
	for (i = 0; i < 6; i++)
	{
		for (unsigned j = 0; j < 3; j++)
		{
			clipVec[2] += (GLfloat)modelMat[2 + j * 4] * pNorms[i][j];
		}
		if (clipVec[2] < 0.0)
			checklist[i] = false;
		clipVec[0] = 0.0;
		clipVec[1] = 0.0;
		clipVec[2] = 0.0;
	}

	//INTERSECTIONS!
	for (i = 0; i < 6; i++)
	{
		if (checklist[i] == false)
			continue;
		double pndotrd = pNorms[i][0]*dir[0] + pNorms[i][1]*dir[1] + pNorms[i][2]*dir[2];
		float point = (float)(-(pNorms[i][0] * nVec[0] + pNorms[i][1] * nVec[1] + pNorms[i][2] * nVec[2] + ofst[i]) / pndotrd);

		// get (x, y, z) intersection on plane
		tPts[i][0] =  (float)(nVec[0] + (point * dir[0]));
		tPts[i][1] =  (float)(nVec[1] + (point * dir[1]));
		tPts[i][2] =  (float)(nVec[2] + (point * dir[2]));
	}

	//Just pick the closet point on the surface
	float closest[3] = { 0.0f, 0.0f, -300.0f };
	//float transClosest[3] = { 0.0f, 0.0f, -300.0f }; //Transformed closest point
	for (i = 0; i < 6; i++)
	{
		if (checklist[i] == false)
			continue;
		if (fabs(tPts[i][0]) <= 3.00001f && fabs(tPts[i][1]) <= 3.00001f && fabs(tPts[i][2]) <= 3.00001f)
		{
			closest[0] = tPts[i][0];
			closest[1] = tPts[i][1];
			closest[2] = tPts[i][2];
			pickedFace = i;
		}
	}
	finalPick.x = closest[0];
	finalPick.y = closest[1];
	finalPick.z = closest[2];

	//return finalPick;
}

void GfxOpenGL::Get3DDeltaPick(int x, int y, unsigned& currentFace, Point& finalPick)
{
	const unsigned& i = currentFace;
	//Point finalPick;
	GLdouble modelMat[16], projMat[16];
	GLint viewport[4];
	GLdouble nVec[3], fVec[3], dir[3];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluUnProject((GLdouble)x, (GLdouble)viewport[3] - (GLdouble)y, (GLdouble)0.0, modelMat, projMat, viewport, &nVec[0], &nVec[1], &nVec[2]);
	gluUnProject((GLdouble)x, (GLdouble)viewport[3] - (GLdouble)y, (GLdouble)1.0, modelMat, projMat, viewport, &fVec[0], &fVec[1], &fVec[2]);
	dir[0] = fVec[0] - nVec[0]; dir[1] = fVec[1] - nVec[1]; dir[2] = fVec[2] - nVec[2];

	float pNorms[6][3] = { {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f},
						   {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f} };
	float ofst[6] = { -3.0f, -3.0f, -3.0f, -3.0f, -3.0f, -3.0f };

	double pndotrd = pNorms[i][0]*dir[0] + pNorms[i][1]*dir[1] + pNorms[i][2]*dir[2];
	float point = (float)(-(pNorms[i][0] * nVec[0] + pNorms[i][1] * nVec[1] + pNorms[i][2] * nVec[2] + ofst[i]) / pndotrd);

	// get (x, y, z) intersection on plane
	finalPick.x =  (float)(nVec[0] + (point * dir[0]));
	finalPick.y =  (float)(nVec[1] + (point * dir[1]));
	finalPick.z =  (float)(nVec[2] + (point * dir[2]));

	//return finalPick;
}
