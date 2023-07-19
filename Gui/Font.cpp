#include "stdafx.h"
#include "Font.h"
#include "RenderMgr.h"
#include "Core/Convert.h"
#include "Win32Dpi.h"
#include "TextMgr.h"

#ifndef FW_NORMAL
// Same as in the Win32 API.
#define FW_DONTCARE		0
#define FW_THIN			100
#define FW_EXTRALIGHT	200
#define FW_ULTRALIGHT 	200
#define FW_LIGHT 	    300
#define FW_NORMAL 		400
#define FW_REGULAR 		400
#define FW_MEDIUM 		500
#define FW_SEMIBOLD 	600
#define FW_DEMIBOLD 	600
#define FW_BOLD 		700
#define FW_EXTRABOLD 	800
#define FW_ULTRABOLD 	800
#define FW_BLACK 		900
#define FW_HEAVY 		900
#endif

namespace gui {

	/**
	 * Shared data between font objects. Not cloned between different threads, and is therefore
	 * protected by explicit locks and atomics. Be careful.
	 * It generally assumes that once something is created, it will not be destroyed again, unless we're
	 * the only reference to an object.
	 */
	struct FontData {
		// Create.
		FontData() : refs(1), backend() {
			init();
		}

		// Destroy.
		~FontData() {
			backend.clear();
			clear();
		}

		// Refcount.
		nat refs;

		// Backend data.
		TextMgr::Resource backend;

		// Increase
		void addRef() {
			atomicIncrement(refs);
		}

		// Decrease.
		void release() {
			if (atomicDecrement(refs) == 0)
				delete this;
		}

		// Invalidate. Returns either this object or a new one, where no data is created.
		FontData *invalidate() {
			if (atomicDecrement(refs) == 0) {
				// We're alone, clear.
				clear();
				addRef();
				return this;
			} else {
				// Not alone. We need to preserve ourselves for the other owner...
				return new FontData();
			}
		}

#ifdef GUI_WIN32
		// WIN32 fonts. DPI->font.
		typedef std::map<Nat, HFONT> FontMap;
		FontMap hFonts;

		void init() {}

		// Clear data. _not_ thread safe, only to be used inside 'invalidate'.
		void clear() {
			for (FontMap::iterator i = hFonts.begin(), end = hFonts.end(); i != end; ++i)
				DeleteObject(i->second);
			hFonts.clear();
		}
#endif
#ifdef GUI_GTK
		// What do we need?
		PangoFontDescription *desc;

		void init() {
			desc = null;
		}

		void clear() {
			if (desc)
				pango_font_description_free(desc);
			desc = null;
		}
#endif

		// Lock for modifying any members.
		os::Lock lock;
	};

	Font::Font(Str *face, Float height) {
		fName = face;
		fHeight = height;
		fWeight = FW_NORMAL;
		fTabWidth = 2 * height;
		fItalic = false;
		fUnderline = false;
		fStrikeOut = false;
		shared = new FontData();
	}

	Font::Font(Str *face, Float height, Float tabWidth) {
		fName = face;
		fHeight = height;
		fWeight = FW_NORMAL;
		fTabWidth = tabWidth;
		fItalic = false;
		fUnderline = false;
		fStrikeOut = false;
		shared = new FontData();
	}

	Font::Font(const Font &f) {
		fName = f.fName;
		fHeight = f.fHeight;
		fWeight = f.fWeight;
		fTabWidth = f.fTabWidth;
		fItalic = f.fItalic;
		fUnderline = f.fUnderline;
		fStrikeOut = f.fStrikeOut;
		shared = f.shared;
		shared->addRef();
	}

	Font::~Font() {
		shared->release();
	}

	void Font::name(Str *name) {
		fName = name;
		changed();
	}

	void Font::height(Float h) {
		fHeight = h;
		changed();
	}

	void Font::weight(Int w) {
		fWeight = w;
		changed();
	}

	void Font::tabWidth(Float w) {
		fTabWidth = w;
		changed();
	}

	void Font::italic(Bool u) {
		fItalic = u;
		changed();
	}

	void Font::underline(Bool u) {
		fUnderline = u;
		changed();
	}

	void Font::strikeOut(Bool u) {
		fStrikeOut = u;
		changed();
	}

	void Font::changed() {
		shared = shared->invalidate();
	}

	void Font::toS(StrBuf *to) const {
		*to << fName << L", " << fHeight << L" pt";
		if (fWeight > FW_NORMAL)
			*to << L", bold";
		if (fItalic)
			*to << L", italic";
		if (fUnderline)
			*to << L", underline";
		if (fStrikeOut)
			*to << L", strike out";
	}

	void *Font::backendFont() const {
		os::Lock::L z(shared->lock);
		if (!shared->backend.data) {
			RenderMgr *mgr = renderMgr(engine());
			shared->backend = mgr->text()->createFont(this);
		}
		return shared->backend.data;
	}

#ifdef GUI_WIN32

	Font::Font(LOGFONT &f) {
		fName = new (this) Str(f.lfFaceName);
		fHeight = (float)abs(f.lfHeight);
		fWeight = (int)f.lfWeight;
		fItalic = f.lfItalic == TRUE;
		fTabWidth = 0;
		fUnderline = f.lfUnderline == TRUE;
		fStrikeOut = f.lfStrikeOut == TRUE;
		shared = new FontData();
	}

	HFONT Font::handle(Nat dpi) {
		os::Lock::L z(shared->lock);
		FontData::FontMap::iterator found = shared->hFonts.find(dpi);
		if (found == shared->hFonts.end()) {
			LOGFONT lf;
			zeroMem(lf);
			lf.lfHeight = LONG(-fHeight * Float(dpi) / Float(defaultDpi));
			lf.lfWidth = 0;
			lf.lfEscapement = 0;
			lf.lfOrientation = 0;
			lf.lfWeight = fWeight;
			lf.lfItalic = fItalic ? TRUE : FALSE;
			lf.lfUnderline = fUnderline ? TRUE : FALSE;
			lf.lfStrikeOut = fStrikeOut ? TRUE : FALSE;
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
			lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			lf.lfQuality = DEFAULT_QUALITY;
			lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
			memcpy(lf.lfFaceName, fName->c_str(), max(sizeof(lf.lfFaceName), wcslen(fName->c_str()) * sizeof(wchar)));
			HFONT hFont = CreateFontIndirect(&lf);
			shared->hFonts.insert(std::make_pair(dpi, hFont));
			return hFont;
		} else {
			return found->second;
		}
	}

	static void addLine(HDC dc, const wchar *start, size_t length, SIZE &update) {
		SIZE size = {0, 0};
		if (length == 0) {
			GetTextExtentPoint32(dc, L"A", 1, &size);
		} else {
			GetTextExtentPoint32(dc, start, int(length), &size);
		}

		update.cx = max(update.cx, size.cx);
		update.cy += size.cy;
	}

	Size Font::stringSize(const Str *str) {
		return stringSize(str, defaultDpi);
	}

	Size Font::stringSize(const Str *str, Nat dpi) {
		return stringSize(str->c_str(), dpi);
	}

	Size Font::stringSize(const wchar *str, Nat dpi) {
		HFONT font = handle(dpi);

		HDC dc = GetDC(NULL);
		HGDIOBJ oldFont = SelectObject(dc, font);


		// We need to consider each line separately.
		SIZE total = {0, 0};
		const wchar *start = str;
		const wchar *at;
		for (at = start; *at; at++) {
			if (*at != '\n')
				continue;

			addLine(dc, start, at - start, total);
			start = at + 1;
		}

		if (start < at) {
			addLine(dc, start, at - start, total);
		}

		SelectObject(dc, oldFont);
		ReleaseDC(NULL, dc);

		return Size(Float(total.cx), Float(total.cy));
	}

#endif

#ifdef GUI_GTK

	Font::Font(const PangoFontDescription &desc) {
		fName = new (this) Str(toWChar(engine(), pango_font_description_get_family(&desc)));
		fHeight = fromPango(pango_font_description_get_size(&desc));
		fWeight = pango_font_description_get_weight(&desc);
		fItalic = pango_font_description_get_style(&desc) == PANGO_STYLE_ITALIC;
		fTabWidth = 0;
		// TODO: How do I get/set these?
		fUnderline = false;
		fStrikeOut = false;
		shared = new FontData();
		shared->desc = pango_font_description_copy(&desc);
	}

	PangoFontDescription *Font::desc() {
		os::Lock::L z(shared->lock);
		if (!shared->desc) {
			shared->desc = pango_font_description_new();
			pango_font_description_set_family(shared->desc, fName->utf8_str());
			pango_font_description_set_size(shared->desc, toPango(fHeight));
			pango_font_description_set_style(shared->desc, fItalic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
			pango_font_description_set_weight(shared->desc, PangoWeight(fWeight));
			// Note: Underline and strikethrough are set when creating text layouts.
			// TODO: Check that when creating labels etc...
		}
		return shared->desc;
	}

#endif

}
