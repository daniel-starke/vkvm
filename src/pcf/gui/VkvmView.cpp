/**
 * @file VkvmView.cpp
 * @author Daniel Starke
 * @date 2019-10-07
 * @version 2020-05-03
 */
#include <cstdlib>
#include <cstring>
#include <FL/fl_ask.H>
#include <libpcf/target.h>
#include <pcf/gui/VkvmView.hpp>
#include <pcf/Utility.hpp>


#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif


namespace pcf {
namespace gui {


/**
 * Constructor.
 *
 * @param[in] X - x coordinate
 * @param[in] Y - y coordinate
 * @param[in] W - width
 * @param[in] H - height
 */
VkvmView::VkvmView(const int X, const int Y, const int W, const int H):
	Fl_Gl_Window(X, Y, W, H),
	capDev(NULL),
	lastImage(NULL),
	lastImageSize(0),
	lastWidth(0),
	lastHeight(0),
	lastFormat(0),
	lastType(0),
	capResizeCb(NULL),
	capResizeCbArg(NULL),
	clickCb(NULL),
	clickCbArg(NULL),
	curRotation(ROT_DEFAULT)
{
	this->set_visible_focus();
	this->end();
}


/**
 * Destructor.
 */
VkvmView::~VkvmView() {
	if (this->capDev != NULL) {
		delete this->capDev;
	}
	if (this->lastImage != NULL) {
		free(this->lastImage);
	}
}


/**
 * Changes the capture device used for display.
 *
 * @param[in] dev - new capture device
 * @return true on success, else false
 */
bool VkvmView::captureDevice(pcf::video::CaptureDevice * dev) {
	bool res = true;
	if (this->capDev != NULL) {
		this->capDev->stop();
		delete this->capDev;
		if (this->lastImage != NULL) {
			free(this->lastImage);
			this->lastImage = NULL;
			this->lastImageSize = 0;
			this->lastWidth = 0;
			this->lastHeight = 0;
			this->lastFormat = 0;
			this->lastType = 0;
			this->doCaptureResizeCallback();
		}
	}
	this->capDev = (dev != NULL) ? dev->clone() : NULL;
	if (this->capDev != NULL) {
		res = this->capDev->start(fl_xid(this->top_window()), *this);
	}
	redraw();
	return res;
}


/**
 * Event handler.
 *
 * @param[in] e - event
 * @return 0 - if the event was not used or understood
 * @return 1 - if the event was used and can be deleted
 */
int VkvmView::handle(int e) {
	const int res = this->Fl_Gl_Window::handle(e);
	switch (e) {
	case FL_ENTER:
	case FL_LEAVE:
		return 1; /* needed to enable tooltip support */
	case FL_PUSH:
		this->doClickCallback();
		break;
	default:
		break;
	}
	return res;
}


/**
 * Draws the widget. This should never be called directly. Use redraw() instead.
 */
void VkvmView::draw() {
	if ( ! visible() ) return;
	const int pw = pixel_w();
	const int ph = pixel_h();
	/* initialize OpenGL */
	if ( ! valid() ) {
		valid(1);
		glLoadIdentity();
		glViewport(0, 0, pw, ph);
		glMatrixMode(GL_PROJECTION);
		glOrtho(0, pw, 0, ph, 1, 0);
		glDisable(GL_LIGHTING);
	}
	/* create texture */
	GLuint texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	this->captureMutex.lock();
	if (this->lastImage != NULL) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->lastWidth, this->lastHeight, 0, this->lastFormat, this->lastType, this->lastImage);
	}
	this->captureMutex.unlock();
	/* update view port */
	if (damage() & FL_DAMAGE_ALL) {
		const int tx = this->mirrorRight() ? 1 : 0;
		const int ty = this->mirrorUp() ? 1 : 0;
		const int rot = int(this->rotation());
		const int txr[4] = {tx, tx, tx ^ 1, tx ^ 1};
		const int tyr[4] = {ty, ty ^ 1, ty ^ 1, ty};
		const int idx = rot ^ ((rot & 1) << 1);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
			glTexCoord2i(txr[(idx + 0) % 4], tyr[(idx + 0) % 4]); glVertex2i(0,  0);
			glTexCoord2i(txr[(idx + 1) % 4], tyr[(idx + 1) % 4]); glVertex2i(0,  ph);
			glTexCoord2i(txr[(idx + 2) % 4], tyr[(idx + 2) % 4]); glVertex2i(pw, ph);
			glTexCoord2i(txr[(idx + 3) % 4], tyr[(idx + 3) % 4]); glVertex2i(pw, 0);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	/* delete texture */
	glDeleteTextures(1, &texId);
}


void VkvmView::onCapture(const pcf::color::Rgb24 * img, const size_t width, const size_t height) {
	this->updateImage(GL_RGB, GL_UNSIGNED_BYTE, static_cast<const GLvoid *>(img), width, height, sizeof(*img) * width * height);
}


void VkvmView::onCapture(const pcf::color::Bgr24 * img, const size_t width, const size_t height) {
	this->updateImage(GL_BGR, GL_UNSIGNED_BYTE, static_cast<const GLvoid *>(img), width, height, sizeof(*img) * width * height);
}


void VkvmView::updateImage(const GLenum format, const GLenum datType, const GLvoid * img, const size_t width, const size_t height, const size_t byteSize) {
	if (img == NULL || width <= 0 || height <= 0) return;
	std::lock_guard<std::mutex> guard(this->captureMutex);
	if (this->lastImageSize != byteSize) {
		if (this->lastImage != NULL) free(this->lastImage);
		this->lastImage = malloc(byteSize);
	}
	if (this->lastImage == NULL) {
		return;
	}
	const bool resized = this->lastWidth != GLsizei(width) || this->lastHeight != GLsizei(height);
	/* copy image data to internal buffer */
	memcpy(this->lastImage, img, byteSize);
	this->lastImageSize = byteSize;
	this->lastWidth = GLsizei(width);
	this->lastHeight = GLsizei(height);
	this->lastFormat = format;
	this->lastType = datType;
	/* update view in event thread */
	Fl::awake([](void * viewPtr){
		if (viewPtr == NULL) return;
		VkvmView * view = static_cast<VkvmView *>(viewPtr);
		view->redraw();
	}, this);
	/* call resize callback */
	if ( resized ) {
		Fl::awake([](void * viewPtr) {
			if (viewPtr == NULL) return;
			VkvmView * view = static_cast<VkvmView *>(viewPtr);
			view->doCaptureResizeCallback();
		}, this);
	}
}


} /* namespace gui */
} /* namespace pcf */
