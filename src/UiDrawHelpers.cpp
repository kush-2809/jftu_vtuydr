#include "SmartTankState.h"
#include "Theme.h"
#include "UiState.h"
#include "MenuModel.h"
#include "WiFiManager.h"
#include "FeatureControl.h"
#include "Management.h"
#include "DisplayOrientation.h"
#include <WiFi.h>

static void drawSelectedRow(int x, int y, int w, int h) {
  const Theme& t = themeGet();
  canvas.fillRoundRect(x, y, w, h, 5, t.selBg);
  canvas.fillRect(x, y, 3, h, t.accent);
}

static String menuItemState(Page page, int index) {
  if (page == Page::Layout) return index == themeIndex() ? "ACTIVE" : "";
  if (page == Page::AdminConfig) {
    switch (menuItemAt(page, index).target) {
      case Page::AdminToggleLoRa: return featureLoraEnabled() ? "ON" : "OFF";
      case Page::AdminToggleSSR: return featureSsrEnabled() ? "ON" : "OFF";
      case Page::AdminToggleTouch: return featureTouchEnabled() ? "ON" : "OFF";
      default: return "";
    }
  }
  if (page == Page::Management && index == 0) return managementModeName(managementMode());
  return "";
}

void uiMenuDraw(const char* title, int /*unused*/) {
  const Theme& t=themeGet();
  const Page page=uiPage();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0,0,W,36,t.headerBg);
  canvas.setTextSize(2);canvas.setTextColor(t.accent,t.headerBg);
  canvas.setCursor(8,10);canvas.print(title);
  canvas.fillRect(0,36,W,2,t.accent);

  const int n=menuItemCount(page),sel=uiMenuIndex();
  const int rowTop=43,rowBottom=H-23,rowHeight=W>H?28:34;
  int visible=(rowBottom-rowTop)/rowHeight;
  if(visible<1)visible=1;
  if(visible>n)visible=n;
  int first=sel>=visible?sel-visible+1:0;
  if(first+visible>n)first=n>visible?n-visible:0;

  for(int slot=0;slot<visible&&first+slot<n;++slot){
    const int index=first+slot,y=rowTop+slot*rowHeight,rowH=rowHeight-4;
    const bool selected=index==sel;
    if(selected)drawSelectedRow(8,y,W-16,rowH);
    const bool locked=page==Page::MainMenu&&menuItemAt(page,index).target==Page::AdminConfig&&!adminUnlocked();
    canvas.setTextSize(2);
    canvas.setTextColor(selected?t.selFg:(locked?t.muted:t.fg),selected?t.selBg:t.bg);
    canvas.setCursor(selected?19:14,y+(rowH-16)/2);
    canvas.print(menuItemAt(page,index).label);
    String state=menuItemState(page,index);
    if(locked)state="LOCKED";
    if(state.length()){
      canvas.setTextSize(1);
      canvas.setTextColor(selected?t.selFg:t.muted,selected?t.selBg:t.bg);
      canvas.setCursor(W-66,y+(rowH-8)/2);canvas.print(state);
    }
  }
  if(n>visible&&visible>0){
    const int trackY=rowTop,trackH=visible*rowHeight-4;
    canvas.drawFastVLine(W-3,trackY,trackH,t.muted);
    int thumbH=(trackH*visible)/n;if(thumbH<8)thumbH=8;
    const int maxFirst=n-visible;
    const int thumbY=trackY+(maxFirst>0?(trackH-thumbH)*first/maxFirst:0);
    canvas.fillRect(W-4,thumbY,3,thumbH,t.accent);
  }
  canvas.fillRect(0,H-20,W,20,t.footerBg);
  canvas.setTextSize(1);canvas.setTextColor(t.fg,t.footerBg);
  canvas.setCursor(7,H-13);canvas.print("ROTATE: MOVE  PRESS: SELECT  HOLD: BACK");
  canvas.pushSprite(0,0);
}

void uiPlaceholderDraw(const char* title, const char* line) {
  const Theme& t = themeGet();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0, 0, W, 36, t.headerBg);
  canvas.setTextSize(2);
  canvas.setTextColor(t.accent, t.headerBg);
  canvas.setCursor(8, 10);
  canvas.print(title);
  canvas.setTextSize(2);
  canvas.setTextColor(t.fg, t.bg);
  canvas.setCursor(12, 90);
  canvas.print(line);
  canvas.fillRect(0, H - 20, W, 20, t.footerBg);
  canvas.setTextSize(1);
  canvas.setTextColor(t.fg, t.footerBg);
  canvas.setCursor(7, H - 13);
  canvas.print("HOLD: BACK");
  canvas.pushSprite(0, 0);
}

void uiDrawScrollIndicator(int maxOffset) {
  if(maxOffset<=0)return;
  const Theme& t=themeGet();
  const int top=42,trackH=H-24-top;
  if(trackH<=0)return;
  int thumbH=trackH*trackH/(trackH+maxOffset);
  if(thumbH<12)thumbH=12;
  if(thumbH>trackH)thumbH=trackH;
  const int thumbY=top+(trackH-thumbH)*uiScrollOffset()/maxOffset;
  canvas.drawFastVLine(W-3,top,trackH,t.muted);
  canvas.fillRect(W-4,thumbY,3,thumbH,t.accent);
}

void uiSystemInfoDraw() {
  const Theme& t=themeGet();
  canvas.fillSprite(t.bg);
  canvas.fillRect(0,0,W,36,t.headerBg);
  canvas.setTextSize(2);canvas.setTextColor(t.accent,t.headerBg);
  canvas.setCursor(8,10);canvas.print("SYSTEM INFO");
  canvas.fillRect(0,36,W,2,t.accent);
  const uint64_t mac=ESP.getEfuseMac();
  char macStr[18],serialStr[13];
  snprintf(macStr,sizeof(macStr),"%02X:%02X:%02X:%02X:%02X:%02X",
    (uint8_t)(mac>>40),(uint8_t)(mac>>32),(uint8_t)(mac>>24),
    (uint8_t)(mac>>16),(uint8_t)(mac>>8),(uint8_t)mac);
  snprintf(serialStr,sizeof(serialStr),"%012llX",(unsigned long long)(mac&0xFFFFFFFFFFFFULL));
  const String serial(serialStr);
  const int scroll=uiScrollOffset();int y=47;
  auto line=[&](const char* key,const String& value){
    const int yy=y-scroll;
    if(yy>=39&&yy<=H-30){
      canvas.setTextSize(1);canvas.setTextColor(t.muted,t.bg);
      canvas.setCursor(8,yy);canvas.print(key);
      canvas.setTextColor(t.fg,t.bg);canvas.setCursor(93,yy);canvas.print(value);
    }y+=18;
  };
  line("Device","SmartTank S3");line("Firmware","v1.1.1");line("MAC",macStr);
  line("Serial",serial);line("IP",wifiManagerGetIP());
  line("Display",String(W)+" x "+String(H));
  line("Orientation",displayRotationName(displayRotation()));
  line("Color depth","RGB565 / 16-bit");
  line("CPU",String(ESP.getCpuFreqMHz())+" MHz");
  line("Heap free",String(ESP.getFreeHeap()/1024)+" KB");
  const int adminY=226-scroll,unlockY=244-scroll;
  if(adminY>=39&&adminY<=H-30){
    canvas.setTextSize(1);canvas.setTextColor(adminUnlocked()?t.ok:t.warn,t.bg);
    canvas.setCursor(8,adminY);canvas.print(adminUnlocked()?"ADMIN ACCESS: UNLOCKED":"ADMIN ACCESS: LOCKED");
  }
  if(unlockY>=39&&unlockY<=H-30){
    canvas.setTextSize(1);canvas.setTextColor(t.muted,t.bg);
    canvas.setCursor(8,unlockY);canvas.print("Unlock: rotate L-R-L-R, then press 5 times");
  }
  uiDrawScrollIndicator(252-(H-24));
  canvas.fillRect(0,H-20,W,20,t.footerBg);
  canvas.setTextColor(t.fg,t.footerBg);canvas.setCursor(7,H-13);
  canvas.print("ROTATE: SCROLL / ADMIN GESTURE  HOLD: BACK");
  canvas.pushSprite(0,0);
}
