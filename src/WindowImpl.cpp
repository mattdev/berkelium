/*  Berkelium Implementation
 *  WindowImpl.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#include "berkelium/Platform.hpp"
#include "ContextImpl.hpp"
#include "RenderWidget.hpp"
#include "WindowImpl.hpp"
#include "MemoryRenderViewHost.hpp"
#include "Root.hpp"
#include "berkelium/WindowDelegate.hpp"
#include "berkelium/Cursor.hpp"

#include "base/file_util.h"
#include "base/file_version_info.h"
#include "base/values.h"
#include "net/base/net_util.h"
#include "chrome/browser/in_process_webkit/dom_storage_context.h"
#include "chrome/browser/in_process_webkit/webkit_context.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/renderer_host/site_instance.h"
#include "chrome/browser/renderer_preferences_util.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/dom_ui/dom_ui.h"
#include "chrome/browser/dom_ui/dom_ui_factory.h"
#include "chrome/browser/renderer_host/render_widget_host_view.h"
#include "chrome/browser/renderer_host/render_view_host_factory.h"
#include "chrome/browser/browser_url_handler.h"
#include "chrome/common/native_web_keyboard_event.h"
#include "chrome/browser/renderer_host/render_process_host.h"
#include "chrome/common/bindings_policy.h"
#include "webkit/glue/context_menu.h"
#include <iostream>

#if BERKELIUM_PLATFORM == PLATFORM_LINUX
#include <gdk/gdkcursor.h>
#endif

namespace Berkelium {
//WindowImpl temp;
void WindowImpl::init(SiteInstance*site, int routing_id) {
    mId = routing_id;
    received_page_title_=false;
    is_crashed_=false;
    mRenderViewHost = RenderViewHostFactory::Create(
        site,
        this,
        routing_id,
        profile()->GetWebKitContext()->
        dom_storage_context()->AllocateSessionStorageNamespaceId());
    host()->AllowBindings(
        BindingsPolicy::EXTERNAL_HOST);
}

WindowImpl::WindowImpl(const Context*otherContext):
        Window(otherContext)
{
    mController = new NavigationController(this, profile());
    mMouseX = 0;
    mMouseY = 0;
    mCurrentURL = GURL("about:blank");
    zIndex = 0;
    init(mContext->getImpl()->getSiteInstance(), MSG_ROUTING_NONE);
    CreateRenderViewForRenderManager(host(), false);
}
WindowImpl::WindowImpl(const Context*otherContext, int routing_id):
        Window(otherContext)
{
    mController = new NavigationController(this, profile());
    mMouseX = 0;
    mMouseY = 0;
    mCurrentURL = GURL("about:blank");
    zIndex = 0;
    init(mContext->getImpl()->getSiteInstance(), routing_id);
    CreateRenderViewForRenderManager(host(), true);
}
WindowImpl::~WindowImpl() {
    RenderViewHost* render_view_host = mRenderViewHost;
    mRenderViewHost = NULL;
    render_view_host->Shutdown();
    delete mController;
}

RenderProcessHost *WindowImpl::process() const {
    return host()->process();
}
RenderWidgetHostView *WindowImpl::view() const {
    if (!host()) {
        return NULL;
    }
    return host()->view();
}
RenderViewHost *WindowImpl::host() const {
    return mRenderViewHost;
}

SiteInstance *WindowImpl::GetSiteInstance() {
    return mContext->getImpl()->getSiteInstance();
}

void WindowImpl::SetIsCrashed(bool state) {
  if (state == is_crashed_)
    return;

  is_crashed_ = state;
}

void WindowImpl::SetIsLoading(bool is_loading) {
    host()->SetIsLoading(is_loading);
}
int WindowImpl::GetBrowserWindowID() const {
    return mController->window_id().id();
}

ViewType::Type WindowImpl::GetRenderViewType()const {
    return ViewType::TAB_CONTENTS;
}

static void MakeNavigateParams(const NavigationEntry& entry, NavigationController::ReloadType reload,
                        ViewMsg_Navigate_Params* params) {
  params->page_id = entry.page_id();
  params->url = entry.url();
  params->referrer = entry.referrer();
  params->transition = entry.transition_type();
  params->state = entry.content_state();
  params->navigation_type = (ViewMsg_Navigate_Params::NavigationType)(int)reload;
  params->request_time = base::Time::Now();
}

Widget *WindowImpl::getWidget() const {
    return static_cast<RenderWidget*>(view());
}


void WindowImpl::setTransparent(bool istrans) {
    SkBitmap bg;
    int bitmap = 0;
    if (istrans) {
        bg.setConfig(SkBitmap::kA1_Config, 1, 1);
        bg.setPixels(&bitmap);
    }
    if (host()) {
        host()->Send(new ViewMsg_SetBackground(host()->routing_id(),bg));
    }
}

void WindowImpl::focus() {
    FrontToBackIter iter = frontIter();
    if (iter != frontEnd()) {
        (*iter)->focus();
        ++iter;
    }
    while (iter != frontEnd()) {
        (*iter)->unfocus();
        ++iter;
    }
    if (getWidget()) {
        getWidget()->focus();
    }
}
void WindowImpl::unfocus() {
    FrontToBackIter iter = frontIter();
    while (iter != frontEnd()) {
        (*iter)->unfocus();
        ++iter;
    }
}

void WindowImpl::mouseMoved(int xPos, int yPos) {
    int oldX = mMouseX, oldY = mMouseY;
    mMouseX = xPos;
    mMouseY = yPos;
    bool notifiedOld = false, notifiedNew = false;

    for (FrontToBackIter iter = frontIter(); iter != frontEnd(); ++iter) {
        Rect r = (*iter)->getRect();
        if (!notifiedOld && r.contains(oldX, oldY)) {
            notifiedOld = true;
            (*iter)->mouseMoved(xPos - r.left(), yPos - r.top());
        } else if (!notifiedNew && r.contains(xPos, yPos)) {
            notifiedNew = true;
            (*iter)->mouseMoved(xPos - r.left(), yPos - r.top());
        }

        if (notifiedOld && notifiedNew)
            break;
    }
}
void WindowImpl::mouseButton(unsigned int buttonID, bool down) {
    Widget *wid = getWidgetAtPoint(mMouseX, mMouseY, true);
    if (wid) {
        (wid)->mouseButton(buttonID, down);
    }
}
void WindowImpl::mouseWheel(int xScroll, int yScroll) {
    Widget *wid = getWidgetAtPoint(mMouseX, mMouseY, true);
    if (wid) {
        wid->mouseWheel(xScroll, yScroll);
    }
}

void WindowImpl::textEvent(const wchar_t* evt, size_t evtLength) {
    FrontToBackIter iter = frontIter();
    if (iter != frontEnd()) {
        (*iter)->textEvent(evt,evtLength);
    }
}
void WindowImpl::keyEvent(bool pressed, int mods, int vk_code, int scancode) {
    FrontToBackIter iter = frontIter();
    if (iter != frontEnd()) {
        (*iter)->keyEvent(pressed, mods, vk_code, scancode);
    }
}



void WindowImpl::resize(int width, int height) {
    SetContainerBounds(gfx::Rect(0, 0, width, height));
}

void WindowImpl::cut() {
    if (host()) host()->Cut();
}
void WindowImpl::copy() {
    if (host()) host()->Copy();
}
void WindowImpl::paste() {
    if (host()) host()->Paste();
}
void WindowImpl::undo() {
    if (host()) host()->Undo();
}
void WindowImpl::redo() {
    if (host()) host()->Redo();
}
void WindowImpl::del() {
    if (host()) host()->Delete();
}
void WindowImpl::selectAll() {
    if (host()) host()->SelectAll();
}

void WindowImpl::refresh() {
    doNavigateTo(mCurrentURL, GURL(), NavigationController::RELOAD);
}

void WindowImpl::stop() {
  if (host()) {
    host()->Stop();
  }
}

void WindowImpl::adjustZoom(int mode) {
  if (host()) {
    host()->Zoom((PageZoom::Function)mode);
  }
}

void WindowImpl::goBack() {
  mController->GoBack();
}

void WindowImpl::goForward() {
  mController->GoForward();
}

bool WindowImpl::canGoBack() const {
  return mController->CanGoBack();
}

bool WindowImpl::canGoForward() const {
  return mController->CanGoForward();
}

void WindowImpl::SetContainerBounds (const gfx::Rect &rc) {
    mRect = rc;
    RenderWidgetHostView* myview = view();
    if (myview) {
        myview->SetSize(this->GetContainerSize());
    }
    RenderViewHost* myhost = host();
    if (myhost) {
        static_cast<MemoryRenderViewHost*>(myhost)->Memory_WasResized();
    }
}

void WindowImpl::executeJavascript(WideString javascript) {
    if (host()) {
        host()->ExecuteJavascriptInWebFrame(std::wstring(), javascript.get<std::wstring>());
    }
}

void WindowImpl::insertCSS(WideString css, WideString id) {
    if (host()) {
        std::string cssUtf8, idUtf8;
        WideToUTF8(css.data(), css.length(), &cssUtf8);
        WideToUTF8(id.data(), id.length(), &idUtf8);
        host()->InsertCSSInWebFrame(std::wstring(), cssUtf8, idUtf8);
    }
}

bool WindowImpl::navigateTo(URLString url) {
    this->mCurrentURL = GURL(url.get<std::string>());
    return doNavigateTo(this->mCurrentURL, GURL(), NavigationController::NO_RELOAD);
}

void WindowImpl::TooltipChanged(const std::wstring& tooltipText) {
   if (mDelegate)
       mDelegate->onTooltipChanged(this, WideString::point_to(tooltipText));
}

bool WindowImpl::doNavigateTo(
        const GURL &newURL,
        const GURL &referrerURL,
        NavigationController::ReloadType reload)
{
    if (view()) {
        view()->Hide();
    }
    mController->LoadEntry(mController->CreateNavigationEntry(
        newURL,
        referrerURL,
        PageTransition::TYPED,
        profile()));
    NavigateToPendingEntry(reload);

    return true;
}

bool WindowImpl::NavigateToPendingEntry(NavigationController::ReloadType reload) {
    const NavigationEntry& entry = *mController->pending_entry();

    if (!host()) {
        return false;  // Unable to create the desired render view host.
    }

    // Navigate in the desired RenderViewHost.
    ViewMsg_Navigate_Params navigate_params;
    MakeNavigateParams(entry, reload, &navigate_params);
    host()->Navigate(navigate_params);
    if (view()) {
        view()->Show();
    }

    return true;
}

void WindowImpl::UpdateMaxPageID(int32 page_id) {
  // Ensure both the SiteInstance and RenderProcessHost update their max page
  // IDs in sync. Only TabContents will also have site instances, except during
  // testing.
  if (GetSiteInstance())
    GetSiteInstance()->UpdateMaxPageID(page_id);
  process()->UpdateMaxPageID(page_id);
}

int32 WindowImpl::GetMaxPageID() {
  if (GetSiteInstance())
    return GetSiteInstance()->max_page_id();
  return 0;
}



Profile* WindowImpl::profile() const{
    //return Root::getSingleton().getProfile();
    return getContextImpl()->profile();
}

ContextImpl *WindowImpl::getContextImpl() const {
    return static_cast<ContextImpl*>(getContext());
}

void WindowImpl::onPaint(Widget *wid,
                         const unsigned char *sourceBuffer,
                         const Rect &sourceBufferRect,
                         size_t numCopyRects,
                         const Rect *copyRects,
                         int dx, int dy, const Rect &scrollRect) {
    if (mDelegate) {
        if (wid) {
            mDelegate->onWidgetPaint(
                this, wid,
                sourceBuffer, sourceBufferRect,
                numCopyRects, copyRects,
                dx, dy, scrollRect);
        } else {
            mDelegate->onPaint(
                this,
                sourceBuffer, sourceBufferRect,
                numCopyRects, copyRects,
                dx, dy, scrollRect);
        }
    }
}

void WindowImpl::onWidgetDestroyed(Widget *wid) {
    if (wid != getWidget()) {
        if (mDelegate) {
            mDelegate->onWidgetDestroyed(this, wid);
        }
    }
    removeWidget(wid);
}

/******* RenderViewHostManager::Delegate *******/

bool WindowImpl::CreateRenderViewForRenderManager(
    RenderViewHost* render_view_host,
    bool remote_view_exists) {

  RenderWidget* rwh_view =
      static_cast<RenderWidget*>(this->CreateViewForWidget(render_view_host));

  // If we're creating a renderview for a window that was opened by the
  //  renderer process remotely, calling CreateRenderView will crash the
  //  renderer by attempting to create a second copy of the window
  if (!remote_view_exists) {
      if (!render_view_host->CreateRenderView(string16()))
      return false;
  }

  // Now that the RenderView has been created, we need to tell it its size.
  rwh_view->SetSize(this->GetContainerSize());
  render_view_host->set_view(rwh_view);

  appendWidget(rwh_view);

//  UpdateMaxPageIDIfNecessary(render_view_host->site_instance(),
//                             render_view_host);
  return true;

}


void WindowImpl::DidStartLoading() {
    SetIsLoading(true);

    if (mDelegate) {
        mDelegate->onLoadingStateChanged(this, true);
    }
}
void WindowImpl::DidStopLoading() {
    SetIsLoading(false);

    if (mDelegate) {
        mDelegate->onLoadingStateChanged(this, false);
    }
}

void WindowImpl::OnAddMessageToConsole(
        const std::wstring& message,
        int32 line_no,
        const std::wstring& source_id)
{
    if (mDelegate) {
        mDelegate->onConsoleMessage(this, WideString::point_to(message),
                                    WideString::point_to(source_id), line_no);
    }
}

void WindowImpl::UpdateCursor(const WebCursor& cursor) {
#if BERKELIUM_PLATFORM == PLATFORM_WINDOWS
    // Normally you'd pass a handle to chrome.dll (or for us, berkelium.dll) to GetCursor
    //  so it could pull out cursor resources, but we don't link resources into berkelium.dll.
    // Also: augh, why is GetCursor not const on windows?
    WebCursor& cursor_ = const_cast<WebCursor&>(cursor);
    HCURSOR cursorHandle = cursor_.GetCursor(0);
    Cursor new_cursor(cursorHandle);
#elif BERKELIUM_PLATFORM == PLATFORM_LINUX
    // Returns an int to avoid including Gdk headers--so we have to cast.
    GdkCursorType cursorType = (GdkCursorType)cursor.GetCursorType();
    GdkCursor* cursorPtr = cursorType == GDK_CURSOR_IS_PIXMAP ? cursor.GetCustomCursor() : NULL;
    Cursor new_cursor(cursorType, cursorPtr);
#elif BERKELIUM_PLATFORM == PLATFORM_MAC
    Cursor new_cursor;
#else
    Cursor new_cursor;
#endif
    if (mDelegate) mDelegate->onCursorUpdated(this, new_cursor);
}

/******* RenderViewHostDelegate *******/

RenderViewHostDelegate::View* WindowImpl::GetViewDelegate() {
    return this;
}
RenderViewHostDelegate::Resource* WindowImpl::GetResourceDelegate() {
    return this;
}
RenderViewHostDelegate::BrowserIntegration* WindowImpl::GetBrowserIntegrationDelegate() {
    return this;
}

RendererPreferences WindowImpl::GetRendererPrefs(Profile*) const {
    RendererPreferences ret;
    renderer_preferences_util::UpdateFromSystemSettings(
        &ret, profile());
    ret.browser_handles_top_level_requests = true;
    return ret;
}

WebPreferences WindowImpl::GetWebkitPrefs() {
    WebPreferences web_prefs;
    web_prefs.experimental_webgl_enabled = true;
    return web_prefs;
}

void WindowImpl::UpdateHistoryForNavigation(
    const GURL& virtual_url,
    const NavigationController::LoadCommittedDetails& details,
    const ViewHostMsg_FrameNavigate_Params& params) {
  if (profile()->IsOffTheRecord())
    return;

  // Add to history service.
  HistoryService* hs = profile()->GetHistoryService(Profile::IMPLICIT_ACCESS);
  if (hs) {
    if (PageTransition::IsMainFrame(params.transition) &&
        virtual_url != params.url) {
      // Hack on the "virtual" URL so that it will appear in history. For some
      // types of URLs, we will display a magic URL that is different from where
      // the page is actually navigated. We want the user to see in history
      // what they saw in the URL bar, so we add the virtual URL as a redirect.
      // This only applies to the main frame, as the virtual URL doesn't apply
      // to sub-frames.
      std::vector<GURL> redirects = params.redirects;
      if (!redirects.empty())
        redirects.back() = virtual_url;
      hs->AddPage(virtual_url, this, params.page_id, params.referrer,
                  params.transition, redirects, details.did_replace_entry);
    } else {
      hs->AddPage(params.url, this, params.page_id, params.referrer,
                  params.transition, params.redirects,
                  details.did_replace_entry);
    }
  }
}

bool WindowImpl::UpdateTitleForEntry(NavigationEntry* entry,
                                     const std::wstring& title) {
  // For file URLs without a title, use the pathname instead. In the case of a
  // synthesized title, we don't want the update to count toward the "one set
  // per page of the title to history."
  string16 final_title;
  bool explicit_set;
  if (entry->url().SchemeIsFile() && title.empty()) {
    final_title = UTF8ToUTF16(entry->url().ExtractFileName());
    explicit_set = false;  // Don't count synthetic titles toward the set limit.
  } else {
    TrimWhitespace(WideToUTF16Hack(title), TRIM_ALL, &final_title);
    explicit_set = true;
  }

  if (final_title == entry->title()) {
    if (mDelegate) {
      std::wstring wtitle(UTF16ToWideHack(final_title));
      mDelegate->onTitleChanged(this, WideString::point_to(wtitle));
    }

    return false;  // Nothing changed, don't bother.
  }

  entry->set_title(final_title);

  // Update the history system for this page.
  if (!profile()->IsOffTheRecord() && !received_page_title_) {
    HistoryService* hs =
        profile()->GetHistoryService(Profile::IMPLICIT_ACCESS);
    if (hs)
      hs->SetPageTitle(entry->virtual_url(), final_title);

    // Don't allow the title to be saved again for explicitly set ones.
    received_page_title_ = explicit_set;
  }

  if (mDelegate) {
    std::wstring wtitle(UTF16ToWideHack(final_title));
    mDelegate->onTitleChanged(this, WideString::point_to(wtitle));
  }

  // Lastly, set the title for the view.
  //view()->SetPageTitle(final_title);

  return true;
}



void WindowImpl::RendererUnresponsive(RenderViewHost* rvh,
                                  bool is_during_unload) {
  if (rvh != host()) {
    return;
  }
  if (mDelegate) mDelegate->onUnresponsive(this);
}
void WindowImpl::RendererResponsive(RenderViewHost* rvh) {
  if (rvh != host()) {
    return;
  }
  if (mDelegate) mDelegate->onResponsive(this);
}

void WindowImpl::RenderViewReady(RenderViewHost* rvh) {
  if (rvh != host()) {
    // Don't notify the world, since this came from a renderer in the
    // background.
    return;
  }

  bool was_crashed = is_crashed();
  SetIsCrashed(false);

  // Restore the focus to the tab (otherwise the focus will be on the top
  // window).
  if (was_crashed)
      view()->Focus();
}

void WindowImpl::RenderViewGone(RenderViewHost* rvh) {
  // Ask the print preview if this renderer was valuable.
  if (rvh != host()) {
    // The pending page's RenderViewHost is gone.
    return;
  }

  SetIsLoading(false);
  SetIsCrashed(true);

  // Tell the view that we've crashed so it can prepare the sad tab page.
  //view()->OnTabCrashed();
  if (mDelegate) mDelegate->onCrashed(this);
}

void WindowImpl::OnUserGesture(){
}
void WindowImpl::OnFindReply(int request_id,
                             int number_of_matches,
                             const gfx::Rect& selection_rect,
                             int active_match_ordinal,
                             bool final_update){
}
void WindowImpl::GoToEntryAtOffset(int offset) {
    std::cout << "GOING TO ENTRY AT OFFSET "<<offset<<std::endl;
    mController->GoToOffset(offset);
}
void WindowImpl::GetHistoryListCount(int* back_list_count,
                                     int* forward_list_count){
  int current_index = mController->last_committed_entry_index();
  *back_list_count = current_index;
  *forward_list_count = mController->entry_count() - current_index - 1;
}
void WindowImpl::OnMissingPluginStatus(int status){
}
void WindowImpl::OnCrashedPlugin(const FilePath& plugin_path) {
    DCHECK(!plugin_path.value().empty());

    std::wstring plugin_name = plugin_path.ToWStringHack();
#if defined(OS_WIN) || defined(OS_MACOSX)
    scoped_ptr<FileVersionInfo> version_info(
        FileVersionInfo::CreateFileVersionInfo(plugin_path));
    if (version_info.get()) {
        const std::wstring& product_name = version_info->product_name();
        if (!product_name.empty()) {
            plugin_name = product_name;
#if defined(OS_MACOSX)
            // Many plugins on the Mac have .plugin in the actual name, which looks
            // terrible, so look for that and strip it off if present.
            const std::wstring plugin_extension(L".plugin");
            if (EndsWith(plugin_name, plugin_extension, true))
                plugin_name.erase(plugin_name.length() - plugin_extension.length());
#endif  // OS_MACOSX
        }
    }
#endif
    if (mDelegate) {
        mDelegate->onCrashedPlugin(this, WideString::point_to(plugin_name));
    }
}
void WindowImpl::OnCrashedWorker(){
    if (mDelegate) {
        mDelegate->onCrashedWorker(this);
    }
}
void WindowImpl::OnDidGetApplicationInfo(
        int32 page_id,
        const webkit_glue::WebApplicationInfo& app_info){
}

void WindowImpl::OnPageContents(
    const GURL& url,
    int renderer_process_id,
    int32 page_id,
    const string16& contents,
    const std::string& language,
    bool page_translatable) {
}

void WindowImpl::OnPageTranslated(
    int32 page_id,
    const std::string& original_lang,
    const std::string& translated_lang,
    TranslateErrors::Type error_type) {
}

void WindowImpl::ProcessExternalHostMessage(const std::string& message,
                                            const std::string& origin,
                                            const std::string& target)
{
    if (mDelegate) {
        std::wstring wide_message(UTF8ToWide(message));
        mDelegate->onExternalHost(this,
                                  WideString::point_to(wide_message),
                                  URLString::point_to(origin),
                                  URLString::point_to(target));
    }
}


void WindowImpl::DidNavigate(RenderViewHost* rvh,
                              const ViewHostMsg_FrameNavigate_Params& params) {
  int extra_invalidate_flags = 0;

  // Update the site of the SiteInstance if it doesn't have one yet.
  if (!GetSiteInstance()->has_site())
    GetSiteInstance()->SetSite(params.url);

  // Need to update MIME type here because it's referred to in
  // UpdateNavigationCommands() called by RendererDidNavigate() to
  // determine whether or not to enable the encoding menu.
  // It's updated only for the main frame. For a subframe,
  // RenderView::UpdateURL does not set params.contents_mime_type.
  // (see http://code.google.com/p/chromium/issues/detail?id=2929 )
  // TODO(jungshik): Add a test for the encoding menu to avoid
  // regressing it again.

  /*if (PageTransition::IsMainFrame(params.transition))
        contents_mime_type_ = params.contents_mime_type;
  */

  NavigationController::LoadCommittedDetails details;
  bool did_navigate = mController->RendererDidNavigate(
      params, extra_invalidate_flags, &details);

  // Update history. Note that this needs to happen after the entry is complete,
  // which WillNavigate[Main,Sub]Frame will do before this function is called.
  if (params.should_update_history) {
    // Most of the time, the displayURL matches the loaded URL, but for about:
    // URLs, we use a data: URL as the real value.  We actually want to save
    // the about: URL to the history db and keep the data: URL hidden. This is
    // what the TabContents' URL getter does.
    UpdateHistoryForNavigation(GetURL(), details, params);
  }

  if (!did_navigate)
    return;  // No navigation happened.

  // DO NOT ADD MORE STUFF TO THIS FUNCTION! Your component should either listen
  // for the appropriate notification (best) or you can add it to
  // DidNavigateMainFramePostCommit / DidNavigateAnyFramePostCommit (only if
  // necessary, please).

  // Run post-commit tasks.
  if (details.is_main_frame) {
// FIXME(pathorn): Look into what TabContents uses these functions for.
      //DidNavigateMainFramePostCommit(details, params);
  }
  //DidNavigateAnyFramePostCommit(details, params);
}

void WindowImpl::UpdateState(RenderViewHost* rvh,
                              int32 page_id,
                              const std::string& state) {
  DCHECK(rvh == host());

  // We must be prepared to handle state updates for any page, these occur
  // when the user is scrolling and entering form data, as well as when we're
  // leaving a page, in which case our state may have already been moved to
  // the next page. The navigation controller will look up the appropriate
  // NavigationEntry and update it when it is notified via the delegate.

  int entry_index = mController->GetEntryIndexWithPageID(
      GetSiteInstance(), page_id);
  if (entry_index < 0)
    return;
  NavigationEntry* entry = mController->GetEntryAtIndex(entry_index);

  if (state == entry->content_state())
    return;  // Nothing to update.
  entry->set_content_state(state);
  mController->NotifyEntryChanged(entry, entry_index);
}

void WindowImpl::NavigationEntryCommitted(NavigationController::LoadCommittedDetails* details) {
	GURL url = details->entry->url();
	const std::string&spec=url.spec();
	if (mDelegate) {
		mDelegate->onAddressBarChanged(this, URLString::point_to(spec));
	}
}

void WindowImpl::UpdateTitle(RenderViewHost* rvh,
                              int32 page_id, const std::wstring& title) {
  // If we have a title, that's a pretty good indication that we've started
  // getting useful data.
  //SetNotWaitingForResponse();

  DCHECK(rvh == host());
  NavigationEntry* entry = mController->GetEntryWithPageID(GetSiteInstance(),
                                                            page_id);
  if (!entry || !UpdateTitleForEntry(entry, title))
    return;
}

void WindowImpl::ShowRepostFormWarningDialog() {
}

void WindowImpl::RunFileChooser(const ViewHostMsg_RunFileChooser_Params&params) {
/* Don't have access to a top level window, since this could be run in a
   windowless environment. Perhaps we need a function to return a native
   window handle or NULL if it does not want to allow file choosers.
*/
/*
  if (!mSelectFileDialog.get())
    mSelectFileDialog = SelectFileDialog::Create(this);
  SelectFileDialog::Type dialog_type =
    multiple_files ? SelectFileDialog::SELECT_OPEN_MULTI_FILE :
                     SelectFileDialog::SELECT_OPEN_FILE;
  mSelectFileDialog->SelectFile(dialog_type, title, default_file,
                                  NULL, 0, FILE_PATH_LITERAL(""),
                                  view()->GetTopLevelNativeWindow(), NULL);
*/
}

void WindowImpl::RequestOpenURL(const GURL& url, const GURL& referrer,
                                 WindowOpenDisposition disposition) {
  bool isNewWindow = false;
  bool cancelDefault = false;

  switch (disposition) {
    case NEW_FOREGROUND_TAB:
    case NEW_BACKGROUND_TAB:
    case NEW_POPUP:
    case NEW_WINDOW:
      isNewWindow = true;
      break;
    default:
      break;
  }

  if (mDelegate) {
    std::string urlstring (url.spec());
    std::string referrerstring (referrer.spec());

    mDelegate->onNavigationRequested(
      this,
      URLString::point_to(urlstring),
      URLString::point_to(referrerstring),
      isNewWindow, cancelDefault
    );
  }

  if (!cancelDefault)
    doNavigateTo(url, referrer, NavigationController::NO_RELOAD);
}

void WindowImpl::DomOperationResponse(const std::string& json_string,
                                       int automation_id) {
}


void WindowImpl::RunJavaScriptMessage(
    const std::wstring& message,
    const std::wstring& default_prompt,
    const GURL& frame_url,
    const int flags,
    IPC::Message* reply_msg,
    bool* did_suppress_message)
{
    bool success = false;
    std::wstring promptstr;

    if (mDelegate) {
        std::string frame_url_str (frame_url.spec());
        WideString prompt = WideString::empty();
        mDelegate->onScriptAlert(
            this, WideString::point_to(message),
            WideString::point_to(default_prompt),
            URLString::point_to(frame_url_str),
            flags, success, prompt
        );
        prompt.get(promptstr);
        mDelegate->freeLastScriptAlert(prompt);
    }
    if (host()) {
        host()->JavaScriptMessageBoxClosed(reply_msg, success, promptstr);
    }
}


/******* RenderViewHostDelegate::Resource *******/

RenderWidgetHostView* WindowImpl::CreateViewForWidget(RenderWidgetHost * render_widget_host) {
    RenderWidget *wid = new RenderWidget(this, mId);
    wid->setHost(render_widget_host);
    return wid;
}

void WindowImpl::DidStartProvisionalLoadForFrame(
        RenderViewHost* render_view_host,
        bool is_main_frame,
        const GURL& url) {
    if (!is_main_frame) {
        return;
    }
    if (render_view_host != static_cast<RenderViewHost*>(host())) {
        return;
    }
    if (mDelegate) {
        this->mCurrentURL = url;
        const std::string&spec=url.spec();
        mDelegate->onStartLoading(this, URLString::point_to(spec));
        mDelegate->onAddressBarChanged(this, URLString::point_to(spec));
    }
}

void WindowImpl::DidStartReceivingResourceResponse(
        const ResourceRequestDetails& details) {
    // See "chrome/browser/renderer_host/resource_request_details.h"
    // for list of accessor functions.
}

void WindowImpl::DidRedirectProvisionalLoad(
    int32 page_id,
    const GURL& source_url,
    const GURL& target_url)
{
    // should use page_id to lookup in history
    NavigationEntry *entry;
    if (page_id == -1)
        entry = mController->pending_entry();
    else
        entry = mController->GetEntryWithPageID(GetSiteInstance(), page_id);
    if (!entry || entry->url() != source_url)
        return;
    entry->set_url(target_url);
}

void WindowImpl::DidRedirectResource(const ResourceRedirectDetails& details) {
    // Only accessor function:
    // details->new_url();
}

void WindowImpl::Close(RenderViewHost* rvh) {
}

void WindowImpl::OnContentBlocked(ContentSettingsType type) {
}

void WindowImpl::OnGeolocationPermissionSet(const GURL& requesting_frame, bool allowed) {
}


void WindowImpl::DidLoadResourceFromMemoryCache(
        const GURL& url,
        const std::string& frame_origin,
        const std::string& main_frame_origin,
        const std::string& security_info) {
}

void WindowImpl::DidFailProvisionalLoadWithError(
        RenderViewHost* render_view_host,
        bool is_main_frame,
        int error_code,
        const GURL& url,
        bool showing_repost_interstitial) {
    if (mDelegate) {
      std::string urlstring (url.spec());
      mDelegate->onProvisionalLoadError(this, URLString::point_to(urlstring),
                                        error_code, is_main_frame);
    }
}

void WindowImpl::DocumentLoadedInFrame() {
    if (mDelegate) {
        mDelegate->onLoad(this);
    }
}



/******* RenderViewHostDelegate::View *******/
void WindowImpl::CreateNewWindow(int route_id, WindowContainerType container_type, const string16&frame_name) {
    // A window shown in popup or tab.
    //WINDOW_CONTAINER_TYPE_NORMAL = 0,
    // A window run as a hidden "background" page.
    //WINDOW_CONTAINER_TYPE_BACKGROUND,
    // A window run as a hidden "background" page that wishes to be started
    // upon browser launch and run beyond the lifetime of the pages that
    // reference it.
    //WINDOW_CONTAINER_TYPE_PERSISTENT,

    // std::cout<<"Created window "<<route_id<<std::endl;
    Context * context = getContext();
    WindowImpl * newWindow = new WindowImpl(context, route_id);

    mNewlyCreatedWindows.insert(
        std::pair<int, WindowImpl*>(route_id, newWindow));
}

void WindowImpl::CreateNewWidget(int route_id, WebKit::WebPopupType popup_type) {
    //WebPopupTypeNone, // Not a popup.
    //WebPopupTypeSelect, // A HTML select (combo-box) popup.
    //WebPopupTypeSuggestion, // An autofill/autocomplete popup.

    // std::cout<<"Created widget "<<route_id<<std::endl;
    RenderWidget* wid = new RenderWidget(this, route_id);
    wid->setHost(new MemoryRenderWidgetHost(this, wid, process(), route_id));
    //wid->set_activatable(activatable); // ???????
    mNewlyCreatedWidgets.insert(
        std::pair<int, RenderWidget*>(route_id, wid));
}
void WindowImpl::ShowCreatedWindow(int route_id,
                                   WindowOpenDisposition disposition,
                                   const gfx::Rect& initial_pos,
                                   bool user_gesture) {
    // std::cout<<"Show Created window "<<route_id<<std::endl;
    std::map<int, WindowImpl*>::iterator iter = mNewlyCreatedWindows.find(route_id);
    assert(iter != mNewlyCreatedWindows.end());
    WindowImpl *newwin = iter->second;
    mNewlyCreatedWindows.erase(iter);
    newwin->host()->Init();

    newwin->resize(initial_pos.width(), initial_pos.height());

    if (!newwin->process()->HasConnection()) {
        // The view has gone away or the renderer crashed. Nothing to do.
        // Fixme: memory leak?
        std::cout<<"Show window process fail "<<route_id<<std::endl;
        return;
    }

    if (mDelegate) {
        Rect rect;
        rect.setFromRect(initial_pos);
        mDelegate->onCreatedWindow(this, newwin, rect);
    }
}
void WindowImpl::ShowCreatedWidget(int route_id,
                                   const gfx::Rect& initial_pos) {
    // std::cout<<"Show Created widget "<<route_id<<std::endl;
    std::map<int, RenderWidget*>::iterator iter = mNewlyCreatedWidgets.find(route_id);
    assert(iter != mNewlyCreatedWidgets.end());
    RenderWidget *wid = iter->second;
    appendWidget(wid);
    mNewlyCreatedWidgets.erase(iter);

    if (!wid->GetRenderWidgetHost()->process()->HasConnection()) {
        // The view has gone away or the renderer crashed. Nothing to do.
        // Fixme: memory leak?
        std::cout<<"Show widget process fail "<<route_id<<std::endl;
        return;
    }

    int thisZ = ++zIndex;

    wid->InitAsPopup(view(), initial_pos);
    wid->GetRenderWidgetHost()->Init();

    wid->SetSize(gfx::Size(initial_pos.width(), initial_pos.height()));
    wid->setPos(initial_pos.x(), initial_pos.y());
    if (mDelegate) {
        mDelegate->onWidgetCreated(this, wid, thisZ);
        mDelegate->onWidgetResize(this, wid,
                                  initial_pos.width(), initial_pos.height());
        mDelegate->onWidgetMove(this, wid,
                                initial_pos.x(), initial_pos.y());
    }

}
void WindowImpl::ShowContextMenu(const ContextMenuParams& params) {
    if (!mDelegate)
      return;

    ContextMenuEventArgs args;
    memset(&args, 0, sizeof(args));

    std::string linkUrl (params.link_url.spec());
    std::string srcUrl (params.src_url.spec());
    std::string pageUrl (params.page_url.spec());
    std::string frameUrl (params.frame_url.spec());

    args.linkUrl = URLString::point_to(linkUrl);
    args.srcUrl = URLString::point_to(srcUrl);
    args.pageUrl = URLString::point_to(pageUrl);
    args.frameUrl = URLString::point_to(frameUrl);
    args.selectedText = WideString::point_to(params.selection_text);

    args.mediaType = ContextMenuEventArgs::MediaTypeNone;
    switch (params.media_type) {
    case WebKit::WebContextMenuData::MediaTypeNone:
        args.mediaType = ContextMenuEventArgs::MediaTypeNone;
        break;
    case WebKit::WebContextMenuData::MediaTypeImage:
        args.mediaType = ContextMenuEventArgs::MediaTypeImage;
        break;
    case WebKit::WebContextMenuData::MediaTypeAudio:
        args.mediaType = ContextMenuEventArgs::MediaTypeAudio;
        break;
    case WebKit::WebContextMenuData::MediaTypeVideo:
        args.mediaType = ContextMenuEventArgs::MediaTypeVideo;
        break;
    default:
        break;
    }
    args.mouseX = params.x;
    args.mouseY = params.y;
    args.isEditable = params.is_editable;
    args.editFlags = ContextMenuEventArgs::CanDoNone;
    if (params.edit_flags & WebKit::WebContextMenuData::CanUndo) {
        args.editFlags |= ContextMenuEventArgs::CanUndo;
    }
    if (params.edit_flags & WebKit::WebContextMenuData::CanRedo) {
        args.editFlags |= ContextMenuEventArgs::CanRedo;
    }
    if (params.edit_flags & WebKit::WebContextMenuData::CanCut) {
        args.editFlags |= ContextMenuEventArgs::CanCut;
    }
    if (params.edit_flags & WebKit::WebContextMenuData::CanCopy) {
        args.editFlags |= ContextMenuEventArgs::CanCopy;
    }
    if (params.edit_flags & WebKit::WebContextMenuData::CanPaste) {
        args.editFlags |= ContextMenuEventArgs::CanPaste;
    }
    if (params.edit_flags & WebKit::WebContextMenuData::CanDelete) {
        args.editFlags |= ContextMenuEventArgs::CanDelete;
    }
    if (params.edit_flags & WebKit::WebContextMenuData::CanSelectAll) {
        args.editFlags |= ContextMenuEventArgs::CanSelectAll;
    }

    mDelegate->onShowContextMenu(this, args);
}
void WindowImpl::StartDragging(const WebDropData& drop_data,
                               WebKit::WebDragOperationsMask allowed_ops,
                               const SkBitmap& image,
                               const gfx::Point& image_offset) {
    // TODO: Add dragging event
    host()->DragSourceSystemDragEnded();
}
void WindowImpl::UpdateDragCursor(WebKit::WebDragOperation operation) {
    // TODO: Add dragging event
}
void WindowImpl::GotFocus() {
    // Useless: just calls this when we hand it an input event.
}
void WindowImpl::TakeFocus(bool reverse) {
}
void WindowImpl::HandleKeyboardEvent(const NativeWebKeyboardEvent& event) {
    // Useless: just calls this when we hand it an input event.
}
void WindowImpl::HandleMouseEvent() {
    // Useless: just calls this when we hand it an input event.
}
void WindowImpl::HandleMouseLeave() {
    // Useless: just calls this when we hand it an input event.
}
void WindowImpl::UpdatePreferredWidth(int pref_width) {
}

void WindowImpl::UpdatePreferredSize(const gfx::Size&) {
}

// Callback to give the browser a chance to handle the specified keyboard
// event before sending it to the renderer.
// Returns true if the |event| was handled. Otherwise, if the |event| would
// be handled in HandleKeyboardEvent() method as a normal keyboard shortcut,
// |*is_keyboard_shortcut| should be set to true.
bool WindowImpl::PreHandleKeyboardEvent(const NativeWebKeyboardEvent& event, bool* is_keyboard_shortcut) {
    *is_keyboard_shortcut = false;
    return false;
}

int WindowImpl::getId() const {
  return mId;
}

}
