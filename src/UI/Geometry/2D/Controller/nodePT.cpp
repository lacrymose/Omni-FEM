#include "UI/geometryShapes.h"

node::node(double xCenter, double yCenter) : rectangleShape(xCenter, yCenter)
{
	xPixel = 0;
	yPixel = 0;
}

void node::setCenterYPixel(int yPix)
{
	yPixel = yPix;
}



void node::setCenterXPixel(int xPix)
{
	xPixel = xPix;
}


int node::getCenterXPixel()
{
	return xPixel;
}


int node::getCenterYPixel()
{
	return yPixel;
}

void node::draw()
{
	glPolygonMode(GL_FRONT, GL_LINE);
	glBegin(GL_POLYGON);
		if(isSelected)
			glColor3f(1.0f, 0.0f, 0.0f);
		else
			glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2i(xPixel - 2, yPixel + 2);
		glVertex2i(xPixel + 2, yPixel + 2);
		glVertex2i(xPixel + 2, yPixel - 2);
		glVertex2i(xPixel - 2, yPixel - 2);
	glEnd();
}



void node::setNodeIndex(int index)
{
	nodeIndex = index;
}



int node::getNodeIndex()
{
	return nodeIndex;
}