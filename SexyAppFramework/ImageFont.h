#ifndef __IMAGEFONT_H__
#define __IMAGEFONT_H__

#include "Font.h"
#include "DescParser.h"
#include "SharedImage.h"

#include <map>
#include <cstdint>

namespace Sexy
{

class SexyAppBase;
class Image;

class CharData
{
public:
	Rect					mImageRect;
	Point					mOffset;
	char					mKerningOffsets[256];
	int						mWidth;
	int						mOrder;

public:
	CharData();
	// 仅对单字节字符（0..255）有效的内距查询，超出范围返回 0，避免对 UTF-8 多字节码点越界
	int						GetKerningOffset(uint32_t theNextCodePoint) const;
};

class FontData;

class FontLayer
{
public:	
	FontData*				mFontData;
	StringVector			mRequiredTags;
	StringVector			mExcludedTags;	
	std::map<uint32_t, CharData> mCharData;		// 以 Unicode 码点为键的字形表（支持中文等多字节字符）
	Color					mColorMult;
	Color					mColorAdd;
	SharedImageRef			mImage;	
	int						mDrawMode;
	Point					mOffset;
	int						mSpacing;
	int						mMinPointSize;
	int						mMaxPointSize;
	int						mPointSize;
	int						mAscent;
	int						mAscentPadding; // How much space is above the avg uppercase char
	int						mHeight;		// 	
	int						mDefaultHeight; // Max height of font character image rects	
	int						mLineSpacingOffset; // This plus height should get added between lines
	int						mBaseOrder;

public:
	FontLayer(FontData* theFontData);
	FontLayer(const FontLayer& theFontLayer);
	CharData* GetCharData(uint32_t value);
};

typedef std::list<FontLayer> FontLayerList;
typedef std::map<std::string, FontLayer*> FontLayerMap;
typedef std::list<Rect> RectList;

class FontData : public DescParser
{
public:
	bool					mInitialized;
	int						mRefCount;
	SexyAppBase*			mApp;		

	int						mDefaultPointSize;
	std::map<uint32_t, uint32_t> mCharMap;		// 以码点为键的字符重映射表（缺省即恒等映射）
	FontLayerList			mFontLayerList;
	FontLayerMap			mFontLayerMap;

	std::string				mSourceFile;
	std::string				mFontErrorHeader;	

public:
	virtual bool			Error(const std::string& theError);

	bool					GetColorFromDataElement(DataElement *theElement, Color &theColor);
	bool					DataToLayer(DataElement* theSource, FontLayer** theFontLayer);
	virtual bool			HandleCommand(const ListDataElement& theParams);
	// 查询重映射后的码点；未定义映射时返回原码点
	uint32_t				MapChar(uint32_t theCodePoint) const;

public:
	FontData();
	virtual ~FontData();

	void					Ref();
	void					DeRef();

	bool					Load(SexyAppBase* theSexyApp, const std::string& theFontDescFileName);
	bool					LoadLegacy(Image* theFontImage, const std::string& theFontDescFileName);
};

class ActiveFontLayer
{
public:
	FontLayer*				mBaseFontLayer;

	Image*					mScaledImage;
	bool					mOwnsImage;
	std::map<uint32_t, Rect> mScaledCharImageRects;		// 以码点为键的缩放字形矩形表

public:
	ActiveFontLayer();
	ActiveFontLayer(const ActiveFontLayer& theActiveFontLayer);
	virtual ~ActiveFontLayer();
};

typedef std::list<ActiveFontLayer> ActiveFontLayerList;

class RenderCommand
{
public:
	Image*					mImage;
	int						mDest[2];
	int						mSrc[4];
	int						mMode;
	Color					mColor;
	RenderCommand*			mNext;
};

typedef std::multimap<int, RenderCommand> RenderCommandMap;

class ImageFont : public Font
{
public:	
	FontData*				mFontData;
	int						mPointSize;
	StringVector			mTagVector;

	bool					mActiveListValid;
	ActiveFontLayerList		mActiveLayerList;
	double					mScale;
	bool					mForceScaledImagesWhite;

public:
	virtual void			GenerateActiveFontLayers();
	virtual void			DrawStringEx(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect* theClipRect, RectList* theDrawnAreas, int* theWidth);
	uint32_t GetMappedChar(uint32_t value);

public:
	ImageFont(SexyAppBase* theSexyApp, const std::string& theFontDescFileName);
	ImageFont(Image *theFontImage); // for constructing your own image font without a file descriptor
	ImageFont(const ImageFont& theImageFont);
	virtual ~ImageFont();

	// Deprecated
	ImageFont(Image* theFontImage, const std::string& theFontDescFileName);
	//ImageFont(const ImageFont& theImageFont, Image* theImage);
	
	virtual int				CharWidth(uint32_t theChar);
	virtual int				CharWidthKern(uint32_t theChar, uint32_t thePrevChar);
	virtual int				StringWidth(const SexyString& theString);
	virtual void			DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& theClipRect);

	virtual Font*			Duplicate();

	virtual void			SetPointSize(int thePointSize);
	virtual int				GetPointSize();
	virtual void			SetScale(double theScale);
	virtual int				GetDefaultPointSize();
	virtual bool			AddTag(const std::string& theTagName);	
	virtual bool			RemoveTag(const std::string& theTagName);
	virtual bool			HasTag(const std::string& theTagName);
	virtual std::string		GetDefine(const std::string& theName);

	virtual void			Prepare();
};

}

#endif //__IMAGEFONT_H__