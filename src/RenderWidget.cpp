/*  Berkelium Implementation
 *  RenderWidget.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "chrome/browser/renderer_host/backing_store_manager.h"
#if defined(OS_LINUX)
#include "webkit/glue/plugins/gtk_plugin_container_manager.h"
#include "webkit/glue/plugins/gtk_plugin_container.h"
#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>
#endif

#if defined (OS_WIN)
#include <windows.h> // for GetTickCount()
#elif defined (OS_POSIX)
#include <sys/time.h>
#include <time.h>
#endif



#include "berkelium/Platform.hpp"
#include "RenderWidget.hpp"
#include "WindowImpl.hpp"

#include <iostream>

namespace Berkelium {

void Widget::destroy(){
    delete this;
}

RenderWidget::RenderWidget(WindowImpl *winImpl, int id) {
    mFocused = true;
    mBacking = NULL;
    mWindow = winImpl;

    mMouseX=mMouseY=0;
    mModifiers=0;

    mId = id;
}

int RenderWidget::getId() const {
    return mId;
}

void RenderWidget::setHost(RenderWidgetHost *host) {
    mHost = host;
}

void RenderWidget::setPos(int x, int y) {
    mRect.set_x(x);
    mRect.set_y(y);
}

Rect RenderWidget::getRect() const {
    Rect ret;
    ret.mLeft = mRect.x();
    ret.mTop = mRect.y();
    ret.mWidth = mRect.width();
    ret.mHeight = mRect.height();
    return ret;
}

RenderWidget::~RenderWidget() {
    if (mBacking) {
        BackingStoreManager::RemoveBackingStore(mHost);
    }
    mWindow->onWidgetDestroyed(this);
}

void RenderWidget::InitAsPopup(RenderWidgetHostView* parent_host_view,
                           const gfx::Rect& pos){
    SetSize(pos.size());
}

  // Returns the associated RenderWidgetHost.
RenderWidgetHost* RenderWidget::GetRenderWidgetHost() const{
    return mHost;
}

  // Notifies the View that it has become visible.
void RenderWidget::DidBecomeSelected(){
}

  // Notifies the View that it has been hidden.
void RenderWidget::WasHidden(){
}

  // Tells the View to size itself to the specified size.
void RenderWidget::SetSize(const gfx::Size& size){
    mRect.set_width(size.width());
    mRect.set_height(size.height());
}

  // Retrieves the native view used to contain plugins and identify the
  // renderer in IPC messages.
gfx::NativeView RenderWidget::GetNativeView(){
    return gfx::NativeView();
}

  // Actually set/take focus to/from the associated View component.
void RenderWidget::Focus(){
    mFocused = true;
}
void RenderWidget::Blur(){
    mFocused = false;
}

  // Returns true if the view is showing
bool RenderWidget::IsShowing(){
    return true;
}
  // Returns true if the View currently has the focus.
bool RenderWidget::HasFocus(){
    return mFocused;
}

  // Shows/hides the view.  These must always be called together in pairs.
  // It is not legal to call Hide() multiple times in a row.
void RenderWidget::Show(){
}
void RenderWidget::Hide(){
}

  // Retrieve the bounds of the View, in screen coordinates.
gfx::Rect RenderWidget::GetViewBounds() const{
    return mRect;
}

  // Sets the cursor to the one associated with the specified cursor_type
void RenderWidget::UpdateCursor(const WebCursor& cursor){
    mWindow->UpdateCursor(cursor);
}

  // Indicates whether the page has finished loading.
void RenderWidget::SetIsLoading(bool is_loading){
}

  // Enable or disable IME for the view.
void RenderWidget::ImeUpdateTextInputState(WebKit::WebTextInputType type, const gfx::Rect& caret_rect){
}

void RenderWidget::ImeCancelComposition() {
}

void RenderWidget::DidUpdateBackingStore(
    const gfx::Rect& scroll_rect, int scroll_dx, int scroll_dy,
    const std::vector<gfx::Rect>& copy_rects) {
}

  // Notifies the View that the renderer has ceased to exist.
void RenderWidget::RenderViewGone(){
}

// Notifies the View that the renderer will be delete soon.
void RenderWidget::WillDestroyRenderWidget(RenderWidgetHost* rwh) {
}

  // Tells the View to destroy itself.
void RenderWidget::Destroy(){
    delete this;
}

  // Tells the View that the tooltip text for the current mouse position over
  // the page has changed.
void RenderWidget::SetTooltipText(const std::wstring& tooltip_text){
    // Chromium absolutely spams us with this event even when the text hasn't changed
    if (mTooltip.compare(tooltip_text) != 0) {
      mTooltip = tooltip_text;
      if (mWindow)
        mWindow->TooltipChanged(tooltip_text);
    }
}

  // Notifies the View that the renderer text selection has changed.
void RenderWidget::SelectionChanged(const std::string& text) {
}

  // Tells the View whether the context menu is showing. This is used on Linux
  // to suppress updates to webkit focus for the duration of the show.
void RenderWidget::ShowingContextMenu(bool showing) {
}

  // Allocate a backing store for this view
BackingStore* RenderWidget::AllocBackingStore(const gfx::Size& size) {
    SetSize(size);

    if (!mBacking) {
        mBacking = BackingStoreManager::GetBackingStore(mHost, size); // will ignore paints on linux!
    }
    return mBacking;
}

// Allocate a video layer for this view.
VideoLayer* RenderWidget::AllocVideoLayer(const gfx::Size& size) {
    // FIXME: See VideoLayerX, etc. Also need to do something if gpu rendering?
    NOTIMPLEMENTED();
    return NULL;
}

#if defined(OS_MACOSX)
  // Display a native control popup menu for WebKit.
void RenderWidget::ShowPopupWithItems(gfx::Rect bounds,
                                  int item_height,
                                  double item_font,
                                  int selected_item,
                                  const std::vector<WebMenuItem>& items,
                                  bool x){
}

  // Get the view's position on the screen.
gfx::Rect RenderWidget::GetWindowRect(){
    return gfx::Rect(0, 0, mRect.width(), mRect.height());
}

  // Get the view's window's position on the screen.
gfx::Rect RenderWidget::GetRootWindowRect(){
    return mRect;
}

  // Set the view's active state (i.e., tint state of controls).
void RenderWidget::SetActive(bool active){
}

void RenderWidget::SetWindowVisibility(bool) {
}

void RenderWidget::WindowFrameChanged() {
}

  // Methods associated with GPU-accelerated plug-in instances.
gfx::PluginWindowHandle RenderWidget::AllocateFakePluginWindowHandle(bool opaque) {
  return plugin_container_manager_.AllocateFakePluginWindowHandle(opaque);
}
void RenderWidget::DestroyFakePluginWindowHandle(gfx::PluginWindowHandle window) {
  plugin_container_manager_.DestroyFakePluginWindowHandle(window);
}
void RenderWidget::AcceleratedSurfaceSetIOSurface(
    gfx::PluginWindowHandle window,
    int32 width, int32 height,
    uint64_t io_surface_id) {
  plugin_container_manager_.SetSizeAndIOSurface(window,
                                                width,
                                                height,
                                                io_surface_id);
}
void RenderWidget::AcceleratedSurfaceSetTransportDIB(
    gfx::PluginWindowHandle window,
    int32 width, int32 height, base::SharedMemoryHandle transport_dib) {
  plugin_container_manager_.SetSizeAndTransportDIB(window,
                                                   width,
                                                   height,
                                                   transport_dib);
}
void RenderWidget::AcceleratedSurfaceBuffersSwapped(gfx::PluginWindowHandle window) {
  // FIXME: What do we do here.
  //[cocoa_view_ drawAcceleratedPluginLayer];
}
  // Draws the current GPU-accelerated plug-in instances into the given context.
void RenderWidget::DrawAcceleratedSurfaceInstances(CGLContextObj context) {
  CGLSetCurrentContext(context);
  gfx::Rect rect = GetWindowRect();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  // Note that we place the origin at the upper left corner with +y
  // going down
  glOrtho(0, rect.width(), rect.height(), 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  plugin_container_manager_.Draw(context);
}

void RenderWidget::AcceleratedSurfaceContextChanged() {
  plugin_container_manager_.ForceTextureReload();
}
#endif

#if defined(OS_LINUX)

static std::map<gfx::PluginWindowHandle, GtkWidget*> activeWidgets;

void RealizeCallback(GtkWidget* widget, void* user_data) {
    gfx::PluginWindowHandle id = (gfx::PluginWindowHandle)user_data;
    if (id)
        gtk_socket_add_id(GTK_SOCKET(widget), id);
}


void RenderWidget::CreatePluginContainer(gfx::PluginWindowHandle id){
    std::cerr<<"CREATED PLUGIN CONTAINER: "<<id<<std::endl;
    assert(activeWidgets.find(id) == activeWidgets.end());

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    activeWidgets.insert(
        std::pair<gfx::PluginWindowHandle, GtkWidget*>(id, window));

    const GURL &current = mWindow->getCurrentURL();
    std::string pluginTitle = "[Plugin from "+current.spec()+"]";
    gtk_window_set_title (GTK_WINDOW(window), pluginTitle.c_str());

    GtkWidget *widget = gtk_plugin_container_new();

    // The Realize callback is responsible for adding the plug into the socket.
    // The reason is 2-fold:
    // - the plug can't be added until the socket is realized, but this may not
    // happen until the socket is attached to a top-level window, which isn't the
    // case for background tabs.
    // - when dragging tabs, the socket gets unrealized, which breaks the XEMBED
    // connection. We need to make it again when the tab is reattached, and the
    // socket gets realized again.
    //
    // Note, the RealizeCallback relies on the plugin_window_to_widget_map_ to
    // have the mapping.
    g_signal_connect(G_OBJECT(widget), "realize",
                     G_CALLBACK(RealizeCallback), (void*)id);

    // Don't destroy the widget when the plug is removed.
    g_signal_connect(G_OBJECT(widget), "plug-removed",
                     G_CALLBACK(gtk_true), NULL);

    gtk_container_add(GTK_CONTAINER(window), widget);
    gtk_widget_show(widget);

}
void RenderWidget::DestroyPluginContainer(gfx::PluginWindowHandle id){
    std::cerr<<"DESTROYED PLUGIN CONTAINER: "<<id<<std::endl;
    std::map<gfx::PluginWindowHandle, GtkWidget*>::iterator iter;
    iter = activeWidgets.find(id);
    if (iter != activeWidgets.end()) {
        GtkWidget * w = iter->second;
        gtk_widget_destroy(w);
        activeWidgets.erase(iter);
    }
}
#endif

  // Moves all plugin windows as described in the given list.
void RenderWidget::MovePluginWindows(
    const std::vector<webkit_glue::WebPluginGeometry>& moves)
{
#ifdef OS_LINUX
    for (size_t i = 0; i < moves.size(); i++) {
        std::map<gfx::PluginWindowHandle, GtkWidget*>::iterator iter;
        iter = activeWidgets.find(moves[i].window);
        if (iter != activeWidgets.end()) {
            gtk_window_resize(
                GTK_WINDOW (iter->second),
                moves[i].window_rect.width(),
                moves[i].window_rect.height());
            if (moves[i].visible) {
                gtk_widget_show (iter->second);
            } else {
                gtk_widget_hide (iter->second);
            }
        }
    }
#endif
}

void RenderWidget::SetVisuallyDeemphasized(bool deemphasized) {
}

// Returns true if the native view, |native_view|, is contained within in the
// widget associated with this RenderWidgetHostView.
bool RenderWidget::ContainsNativeView(gfx::NativeView native_view) const {
    return false;
}

void RenderWidget::focus() {
    if (mHost) {
        mHost->Focus();
	}
}
void RenderWidget::unfocus() {
    if (mHost) {
        mHost->Blur();
	}
}

bool RenderWidget::hasFocus() const {
    return mFocused;
}

template<class T>
void zeroWebEvent(T &event, int modifiers, WebKit::WebInputEvent::Type t) {
    memset(&event,0,sizeof(T));
    event.type=t;
    event.size=sizeof(T);
    event.modifiers=modifiers;
#ifdef _WIN32
    event.timeStampSeconds=GetTickCount()/1000.0;
#else
    timeval tv;
    gettimeofday(&tv,NULL);
    event.timeStampSeconds=((double)tv.tv_usec)/1000000.0;
    event.timeStampSeconds+=tv.tv_sec;
#endif
}

void RenderWidget::mouseMoved(int xPos, int yPos) {
    WebKit::WebMouseEvent event;
    zeroWebEvent(event, mModifiers, WebKit::WebInputEvent::MouseMove);
	event.x = xPos;
	event.y = yPos;
	event.globalX = xPos+mRect.x();
	event.globalY = yPos+mRect.y();
    mMouseX=xPos;
    mMouseY=yPos;

	event.button = (WebKit::WebMouseEvent::Button)mButton;
    event.clickCount = (mButton != (int32)WebKit::WebMouseEvent::ButtonNone);

	if (GetRenderWidgetHost()) {
		GetRenderWidgetHost()->ForwardMouseEvent(event);
	}
}

void RenderWidget::mouseWheel(int scrollX, int scrollY) {
	WebKit::WebMouseWheelEvent event;
	zeroWebEvent(event, mModifiers, WebKit::WebInputEvent::MouseWheel);
	event.x = mMouseX;
	event.y = mMouseY;
	event.windowX = mMouseX; // PRHFIXME: Window vs Global position?
	event.windowY = mMouseY;
	event.globalX = mMouseX+mRect.x();
	event.globalY = mMouseY+mRect.y();
	event.deltaX = scrollX; // PRHFIXME: want x and y scroll.
	event.deltaY = scrollY;
	event.wheelTicksX = scrollX; // PRHFIXME: want x and y scroll.
	event.wheelTicksY = scrollY;
	event.scrollByPage = false;

	event.button = (WebKit::WebMouseEvent::Button)mButton;
    event.clickCount = (mButton != (int32)WebKit::WebMouseEvent::ButtonNone);

	if (GetRenderWidgetHost()) {
		GetRenderWidgetHost()->ForwardWheelEvent(event);
	}
}

void RenderWidget::mouseButton(unsigned int mouseButton, bool down) {
    unsigned int buttonChangeMask=0;
    switch(mouseButton) {
      case 0:
        buttonChangeMask = WebKit::WebInputEvent::LeftButtonDown;
        break;
      case 1:
        buttonChangeMask = WebKit::WebInputEvent::MiddleButtonDown;
        break;
      case 2:
        buttonChangeMask = WebKit::WebInputEvent::RightButtonDown;
        break;
    }
    if (down) {
        mModifiers|=buttonChangeMask;
    }else {
        mModifiers&=(~buttonChangeMask);
    }
    WebKit::WebMouseEvent event;
    zeroWebEvent(event, mModifiers, down?WebKit::WebInputEvent::MouseDown:WebKit::WebInputEvent::MouseUp);
    switch(mouseButton) {
      case 0:
        event.button = WebKit::WebMouseEvent::ButtonLeft;
        break;
      case 1:
        event.button = WebKit::WebMouseEvent::ButtonMiddle;
        break;
      case 2:
        event.button = WebKit::WebMouseEvent::ButtonRight;
        break;
    }
    if (down){
        event.clickCount=1;
        this->mButton = event.button;
    } else {
        this->mButton = WebKit::WebMouseEvent::ButtonNone;
    }
	event.x = mMouseX;
	event.y = mMouseY;
	event.windowX = mMouseX;
	event.windowY = mMouseY;
	event.globalX = mMouseX+mRect.x();
	event.globalY = mMouseY+mRect.y();
	if (GetRenderWidgetHost()) {
		GetRenderWidgetHost()->ForwardMouseEvent(event);
	}
}

void RenderWidget::keyEvent(bool pressed, int modifiers, int vk_code, int scancode){
	NativeWebKeyboardEvent event;
	zeroWebEvent(event, mModifiers, pressed?WebKit::WebInputEvent::RawKeyDown:WebKit::WebInputEvent::KeyUp);
	event.windowsKeyCode = vk_code;
	event.nativeKeyCode = scancode;
	event.text[0]=0;
	event.unmodifiedText[0]=0;
	event.isSystemKey = (modifiers & Berkelium::SYSTEM_KEY)?true:false;

	event.modifiers=0;
	if (modifiers & Berkelium::ALT_MOD)
		event.modifiers |= WebKit::WebInputEvent::AltKey;
	if (modifiers & Berkelium::CONTROL_MOD)
		event.modifiers |= WebKit::WebInputEvent::ControlKey;
	if (modifiers & Berkelium::SHIFT_MOD)
		event.modifiers |= WebKit::WebInputEvent::ShiftKey;
	if (modifiers & Berkelium::META_MOD)
		event.modifiers |= WebKit::WebInputEvent::MetaKey;
	if (modifiers & Berkelium::KEYPAD_KEY)
		event.modifiers |= WebKit::WebInputEvent::IsKeyPad;
	if (modifiers & Berkelium::AUTOREPEAT_KEY)
		event.modifiers |= WebKit::WebInputEvent::IsAutoRepeat;

	event.setKeyIdentifierFromWindowsKeyCode();

	if (GetRenderWidgetHost()) {
		GetRenderWidgetHost()->ForwardKeyboardEvent(event);
	}
	// keep track of persistent modifiers.
    unsigned int test=(WebKit::WebInputEvent::LeftButtonDown|WebKit::WebInputEvent::MiddleButtonDown|WebKit::WebInputEvent::RightButtonDown);
	mModifiers = ((mModifiers&test) |  (event.modifiers& (Berkelium::SHIFT_MOD|Berkelium::CONTROL_MOD|Berkelium::ALT_MOD|Berkelium::META_MOD)));
}


void RenderWidget::textEvent(const wchar_t * text, size_t textLength) {
    textEvent(WideString::point_to(text, textLength));
}

void RenderWidget::textEvent(WideString wideText) {
	// generate one of these events for each lengthCap chunks.
	// 1 less because we need to null terminate.
    if (!GetRenderWidgetHost()) {
		return;
	}

    string16 text16;
    if (!WideToUTF16(wideText.data(), wideText.length(), &text16)) {
        // error in conversion.
        return;
    }

    // assert(WebKit::WebKeyboardEvent::textLengthCap > 2);

	NativeWebKeyboardEvent event;
	zeroWebEvent(event,mModifiers,WebKit::WebInputEvent::Char);
	event.isSystemKey = false;
	event.windowsKeyCode = 0;
	event.nativeKeyCode = 0;
	event.keyIdentifier[0]=0;
	size_t i;
	for (i = 0; i < text16.length(); i++) {
        event.text[0] = event.unmodifiedText[0] = text16[i];
        if (event.text[0] >= 0xD800 && event.text[0] < 0xDC00 && i < text16.length() - 1) {
            // Surrogate pairs are the only case with two utf-16 chars.
            event.text[1] = event.unmodifiedText[1] = text16[i + 1];
            event.text[2] = event.unmodifiedText[1] = 0;
            i++;
        } else {
            // Otherwise, only send one at a time.
            event.text[1] = event.unmodifiedText[1] = 0;
        }
        GetRenderWidgetHost()->ForwardKeyboardEvent(event);
	}
}

}
