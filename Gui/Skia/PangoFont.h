#pragma once

#include "Skia.h"

#ifdef GUI_ENABLE_SKIA

#include <harfbuzz/hb.h>
#include <fontconfig/fontconfig.h>

namespace gui {

	class SkPangoFontCache;

	/**
	 * Typeface properties. A key in the cache.
	 */
	class SkTypefaceKey {
	public:
		// Create, extract information from Fontconfig. Takes ownership of "blob", i.e. assumes that it is ref'd.
		SkTypefaceKey(hb_blob_t *blob, int index, FcPattern *pattern = null);

		// Copy.
		SkTypefaceKey(const SkTypefaceKey &o);

		// Assign.
		SkTypefaceKey &operator =(const SkTypefaceKey &o);

		// Destroy.
		~SkTypefaceKey();

		// Blob (we have ownership of it).
		hb_blob_t *blob;

		// Typeface index inside the blob.
		int index;

		// Should we fake bold.
		bool embolden;

		// Should we apply a transform?
		FcMatrix transform;

		// Compare for equality.
		bool operator ==(const SkTypefaceKey &key) const {
			return blob == key.blob
				&& index == key.index
				&& embolden == key.embolden
				&& transform.xx == key.transform.xx
				&& transform.xy == key.transform.xy
				&& transform.yx == key.transform.yx
				&& transform.yy == key.transform.yy;
		}

		// Hash.
		size_t hash() const {
			return std::hash<hb_blob_t *>()(blob)
				^ std::hash<int>()(index)
				^ std::hash<bool>()(embolden)
				^ std::hash<float>()(transform.xx)
				^ std::hash<float>()(transform.xy)
				^ std::hash<float>()(transform.yx)
				^ std::hash<float>()(transform.yy);
		}
	};

	// For debugging:
	std::wostream &operator <<(std::wostream &to, const SkTypefaceKey &key);

}

namespace std {
	template <>
	struct hash<gui::SkTypefaceKey> {
		size_t operator ()(const gui::SkTypefaceKey &key) const {
			return key.hash();
		}
	};
}

namespace gui {

	/**
	 * A typeface (i.e. font file, essentially).
	 *
	 * This is an extension of the FreeType base class in Skia, so that we can properly apply the
	 * transformations from Fontconfig (the default implementation using fontconfig does not do that
	 * properly either, it does not scale bitmap fonts even if asked).
	 *
	 * This class also stores the number of users from the GUI library, so that the cache may be
	 * emptied when necessary. This is potentially important as some fonts require us to create
	 * typefaces for particular font sizes in some cases. While it would not be too important to
	 * evict individual fonts (there are only so many), fonts * sizes would be a bit too much,
	 * especially since the fonts requiring this treatment are usually bitmap fonts.
	 */
	class SkPangoTypeface : public SkTypeface_FreeType {
	public:
		// Create a typeface.
		static sk_sp<SkPangoTypeface> load(const SkTypefaceKey &key);

		// The key used to create this typeface.
		SkTypefaceKey key;

		// Number of users in Storm.
		size_t users;

		// Destroy.
		~SkPangoTypeface() {}

	protected:
		// Various functions needed.
		void onGetFamilyName(SkString *familyName) const;
		void onGetFontDescriptor(SkFontDescriptor *desc, bool *serialize) const;
		std::unique_ptr<SkStreamAsset> onOpenStream(int *ttcIndex) const;
		std::unique_ptr<SkFontData> onMakeFontData() const;
		sk_sp<SkTypeface> onMakeClone(const SkFontArguments &args) const;

		// Apply transform etc.
		void onFilterRec(SkScalerContextRec *rec) const;

	private:
		// Internal constructor.
		SkPangoTypeface(std::unique_ptr<SkFontData> data, SkString family,
						const SkFontStyle &style, bool fixedWidth,
						const SkTypefaceKey &key);

		// Font family.
		SkString family;

		// Font data.
		std::unique_ptr<const SkFontData> data;
	};


	/**
	 * Pango font for Skia. Tracks usage so that the system knows when to deallocate the font.
	 *
	 * Implemented to be used with sk_sp to track usage.
	 */
	class SkPangoFont : public SkNVRefCnt<SkPangoFont> {
		friend class SkPangoFontCache;
	public:
		// Destroy.
		~SkPangoFont();

		// Skia font.
		SkFont font;

	private:
		// Create a font.
		SkPangoFont(SkPangoFontCache *cache, PangoFont *font);

		// Reference to the pango typeface inside the font, so that we can remove it from the cache
		// when we are destroyed if required.
		sk_sp<SkPangoTypeface> typeface;

		// The cache we belong to. Set to null whenever the cache is destroyed.
		SkPangoFontCache *cache;

		// Original Pango font. We keep a reference to it.
		PangoFont *pango;
	};

	/**
	 * Font cache.
	 */
	class SkPangoFontCache {
		friend class SkPangoFont;
	public:
		// Create.
		SkPangoFontCache();

		// Destroy.
		~SkPangoFontCache();

		// Get a font.
		sk_sp<SkPangoFont> font(PangoFont *font);

	private:
		// Lock.
		os::Lock lock;

		// Remove a font.
		void remove(PangoFont *font);

		// Get a typeface.
		sk_sp<SkPangoTypeface> typeface(const SkTypefaceKey &key);

		// Remove a typeface.
		void remove(SkPangoTypeface *typeface);

		// Get fonts. We don't own any of the pointers here.
		typedef std::unordered_map<PangoFont *, SkPangoFont *> FontMap;
		FontMap fonts;

		// Get typefaces.
		typedef std::unordered_map<SkTypefaceKey, sk_sp<SkPangoTypeface>> TypefaceMap;
		TypefaceMap typefaces;

		// Load a font into a hb_blob object. Used as a fallback when we don't have a new enough
		// version of Pango.
		void loadBlob(FcPattern *pattern, hb_blob_t *&blob, int &index);

		/**
		 * Simple data structure for loaded hb_blob_t:s. Used to keep track of their lifetime.
		 */
		struct Blob {
			// File used as a key in the map.
			std::string file;

			// Allocated blob. This keeps track of the refcount and ensures this struct is
			// destroyed. Therefore, the blob is not ref'd.
			hb_blob_t *blob;

			// Data allocated for the blob.
			char *data;

			// Owning font cache. Set to null when the cache is destroyed.
			SkPangoFontCache *cache;
		};

		// Destroy a blob structure.
		static void destroyBlob(void *blob);

		// Cache of font blobs. Only used if we fail to extract hb_face_t from Pango.
		// We never deallocate these as we don't keep refcounts to them. That should be fine though,
		// as it looks like Pango does the same. Anyhow, on new versions of Pango, we don't need
		// the fallback, so it should be fine.
		typedef std::unordered_map<std::string, Blob *> BlobMap;
		BlobMap blobs;
	};

}

#endif
