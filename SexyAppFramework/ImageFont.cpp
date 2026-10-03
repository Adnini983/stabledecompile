#include "ImageFont.h"
#include "Graphics.h"
#include "Image.h"
#include "SexyAppBase.h"
#include "MemoryImage.h"
#include "..\SexyAppFramework\AutoCrit.h"
#include "SDL3Image.h"
#include "../LawnApp.h"

using namespace Sexy;

////

DataElement::DataElement() :
	mIsList(false)
{	
}

DataElement::~DataElement()
{
}

SingleDataElement::SingleDataElement()
{	
	mIsList = false;
}

SingleDataElement::SingleDataElement(const std::string theString) :	
	mString(theString)
{	
	mIsList = false;
}

SingleDataElement::~SingleDataElement()
{	
}

DataElement* SingleDataElement::Duplicate()
{
	SingleDataElement* aSingleDataElement = new SingleDataElement(*this);
	return aSingleDataElement;
}

ListDataElement::ListDataElement()	
{	
	mIsList = true;
}

ListDataElement::~ListDataElement()
{
	for (ulong i = 0; i < mElementVector.size(); i++)
		delete mElementVector[i];
}

ListDataElement::ListDataElement(const ListDataElement& theListDataElement)
{
	mIsList = true;
	for (ulong i = 0; i < theListDataElement.mElementVector.size(); i++)
		mElementVector.push_back(theListDataElement.mElementVector[i]->Duplicate());
}

ListDataElement& ListDataElement::operator=(const ListDataElement& theListDataElement)
{
	ulong i;

	for (i = 0; i < mElementVector.size(); i++)
		delete mElementVector[i];
	mElementVector.clear();

	for (i = 0; i < theListDataElement.mElementVector.size(); i++)
		mElementVector.push_back(theListDataElement.mElementVector[i]->Duplicate());

	return *this;
}

DataElement* ListDataElement::Duplicate()
{
	ListDataElement* aListDataElement = new ListDataElement(*this);
	return aListDataElement;
}

///

CharData::CharData()
{
	mWidth = 0;
	mOrder = 0;

	for (ulong i = 0; i < 256; i++)
		mKerningOffsets[i] = 0;
}

int CharData::GetKerningOffset(uint32_t theNextCodePoint) const
{
	// 内距只对单字节字符有意义；中文等多字节码点没有内距，返回 0
	if (theNextCodePoint >= 256)
		return 0;
	return mKerningOffsets[theNextCodePoint];
}

uint32_t FontData::MapChar(uint32_t theCodePoint) const
{
	std::map<uint32_t, uint32_t>::const_iterator aItr = mCharMap.find(theCodePoint);
	if (aItr == mCharMap.end())
		return theCodePoint;
	return aItr->second;
}

// 将单个 UTF-8 字符解码为一个 Unicode 码点；只有当字符串恰好是一个完整字符时才返回 true
static bool TodUtf8SingleChar(const std::string& s, uint32_t& theCodePoint)
{
	if (s.empty())
		return false;
	// 先解除常见转义（\' \" \\），它们表示单个字符
	std::string u;
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == '\\' && i + 1 < s.size())
		{
			u += s[i + 1];
			++i;
		}
		else
			u += s[i];
	}
	// 字体描述用成对引号表示字面引号（'' → '，"" → "）
	if (u == "''")
		u = "'";
	else if (u == "\"\"")
		u = "\"";
	size_t aPos = 0;
	theCodePoint = Sexy::Utf8Decode(u, aPos);
	return aPos == u.size();
}

FontLayer::FontLayer(FontData* theFontData)
{	
	mFontData = theFontData;	
	mDrawMode = -1;
	mSpacing = 0;
	mPointSize = 0;
	mAscent = 0;
	mAscentPadding = 0;
	mMinPointSize = -1;
	mMaxPointSize = -1;	
	mHeight = 0;
	mDefaultHeight = 0;
	mColorMult = Color::White;
	mColorAdd = Color(0, 0, 0, 0);
	mLineSpacingOffset = 0;
	mBaseOrder = 0;
}

FontLayer::FontLayer(const FontLayer& theFontLayer) :	
	mFontData(theFontLayer.mFontData),
	mRequiredTags(theFontLayer.mRequiredTags),
	mExcludedTags(theFontLayer.mExcludedTags),
	mImage(theFontLayer.mImage),
	mDrawMode(theFontLayer.mDrawMode),
	mOffset(theFontLayer.mOffset),
	mSpacing(theFontLayer.mSpacing),
	mMinPointSize(theFontLayer.mMinPointSize),
	mMaxPointSize(theFontLayer.mMaxPointSize),
	mPointSize(theFontLayer.mPointSize),
	mAscent(theFontLayer.mAscent),
	mAscentPadding(theFontLayer.mAscentPadding),
	mHeight(theFontLayer.mHeight),
	mDefaultHeight(theFontLayer.mDefaultHeight),
	mColorMult(theFontLayer.mColorMult),
	mColorAdd(theFontLayer.mColorAdd),
	mLineSpacingOffset(theFontLayer.mLineSpacingOffset),
	mBaseOrder(theFontLayer.mBaseOrder)
{
	// 深拷贝整张字形表（含中文等多字节码点）
	mCharData = theFontLayer.mCharData;
}
CharData* FontLayer::GetCharData(uint32_t value)
{
	return &this->mCharData[value];
}

FontData::FontData()
{
	mInitialized = false;

	mApp = NULL;
	mRefCount = 0;
	mDefaultPointSize = 0;
}

FontData::~FontData()
{
	DataElementMap::iterator anItr = mDefineMap.begin();
	while (anItr != mDefineMap.end())
	{
		std::string aDefineName = anItr->first;
		DataElement* aDataElement = anItr->second;

		delete aDataElement;
		++anItr;
	}
}

void FontData::Ref()
{
	mRefCount++;
}

void FontData::DeRef()
{
	if (--mRefCount == 0)
	{
		delete this;
	}
}

bool FontData::Error(const std::string& theError)
{
	if (mApp != NULL)
	{
		std::string anErrorString = mFontErrorHeader + theError;

		if (mCurrentLine.length() > 0)
		{
			anErrorString += " on Line " + StrFormat("%d:\r\n\r\n", mCurrentLineNum) + mCurrentLine;
		}

		mApp->Popup(anErrorString);
	}

	return false;
}

bool FontData::DataToLayer(DataElement* theSource, FontLayer** theFontLayer)
{
	*theFontLayer = NULL;

	if (theSource->mIsList)
		return false;

	std::string aLayerName = StringToUpper(((SingleDataElement*) theSource)->mString);

	FontLayerMap::iterator anItr = mFontLayerMap.find(aLayerName);
	if (anItr == mFontLayerMap.end())
	{
		Error("Undefined Layer");
		return false;
	}

	*theFontLayer = anItr->second;

	return true;
}

bool FontData::GetColorFromDataElement(DataElement *theElement, Color &theColor)
{
	if (theElement->mIsList)
	{				
		DoubleVector aFactorVector;					
		// 豆包修复：原逻辑用 &&（转换失败且数量恰为 4 才失败）是反的，
		// 转换失败或数量不足 4 时仍会越界访问 aFactorVector[0..3]。
		if (!DataToDoubleVector(theElement, &aFactorVector) || (aFactorVector.size() != 4))
			return false;

		theColor = Color(
			(int) (aFactorVector[0] * 255), 
			(int) (aFactorVector[1] * 255), 
			(int) (aFactorVector[2] * 255), 
			(int) (aFactorVector[3] * 255));

		return true;
	}

	int aColor = 0;
	if (!StringToInt(((SingleDataElement*) theElement)->mString, &aColor))
		return false;

	theColor = aColor;
	return true;
}


bool FontData::HandleCommand(const ListDataElement& theParams)	
{
	std::string aCmd = ((SingleDataElement*) theParams.mElementVector[0])->mString;

	bool invalidNumParams = false;
	bool invalidParamFormat = false;
	bool literalError = false;
	bool sizeMismatch = false;

	if (stricmp(aCmd.c_str(), "Define") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			if (!theParams.mElementVector[1]->mIsList)
			{
				std::string aDefineName = StringToUpper(((SingleDataElement*) theParams.mElementVector[1])->mString);
			
				if (!IsImmediate(aDefineName))
				{
					DataElementMap::iterator anItr = mDefineMap.find(aDefineName);
					if (anItr != mDefineMap.end())
					{
						delete anItr->second;
						mDefineMap.erase(anItr);
					}

					if (theParams.mElementVector[2]->mIsList)
					{
						ListDataElement* aValues = new ListDataElement();
						if (!GetValues(((ListDataElement*) theParams.mElementVector[2]), aValues))
						{
							delete aValues;
							return false;
						}

						mDefineMap.insert(DataElementMap::value_type(aDefineName, aValues));
					}
					else
					{							
						SingleDataElement* aDefParam = (SingleDataElement*) theParams.mElementVector[2];

						DataElement* aDerefVal = Dereference(aDefParam->mString);

						if (aDerefVal)
							mDefineMap.insert(DataElementMap::value_type(aDefineName, aDerefVal->Duplicate()));
						else
							mDefineMap.insert(DataElementMap::value_type(aDefineName, aDefParam->Duplicate()));
					}
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "CreateHorzSpanRectList") == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{	
			IntVector aRectIntVector;
			IntVector aWidthsVector;														

			if ((!theParams.mElementVector[1]->mIsList) &&
				(DataToIntVector(theParams.mElementVector[2], &aRectIntVector)) && 
				(aRectIntVector.size() == 4) &&
				(DataToIntVector(theParams.mElementVector[3], &aWidthsVector)))
			{
				std::string aDefineName = StringToUpper(((SingleDataElement*) theParams.mElementVector[1])->mString);	

				int aXPos = 0;

				ListDataElement* aRectList = new ListDataElement();

				for (ulong aWidthNum = 0; aWidthNum < aWidthsVector.size(); aWidthNum++)
				{							
					ListDataElement* aRectElement = new ListDataElement();
					aRectList->mElementVector.push_back(aRectElement);

					char aStr[256];

					sprintf(aStr, "%d", aRectIntVector[0] + aXPos);
					aRectElement->mElementVector.push_back(new SingleDataElement(aStr));

					sprintf(aStr, "%d", aRectIntVector[1]);
					aRectElement->mElementVector.push_back(new SingleDataElement(aStr));

					sprintf(aStr, "%d", aWidthsVector[aWidthNum]);
					aRectElement->mElementVector.push_back(new SingleDataElement(aStr));

					sprintf(aStr, "%d", aRectIntVector[3]);
					aRectElement->mElementVector.push_back(new SingleDataElement(aStr));								

					aXPos += aWidthsVector[aWidthNum];
				}
				
				DataElementMap::iterator anItr = mDefineMap.find(aDefineName);
				if (anItr != mDefineMap.end())
				{
					delete anItr->second;
					mDefineMap.erase(anItr);
				}

				mDefineMap.insert(DataElementMap::value_type(aDefineName, aRectList));
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "SetDefaultPointSize") == 0)
	{
		if (theParams.mElementVector.size() == 2)
		{					
			int aPointSize;

			if ((!theParams.mElementVector[1]->mIsList) &&
				(StringToInt(((SingleDataElement*) theParams.mElementVector[1])->mString, &aPointSize)))
			{												
				mDefaultPointSize = aPointSize;						
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "SetCharMap") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			StringVector aFromVector;
			StringVector aToVector;

			if ((DataToStringVector(theParams.mElementVector[1], &aFromVector)) &&
				(DataToStringVector(theParams.mElementVector[2], &aToVector)))
			{						
				if (aFromVector.size() == aToVector.size())
				{
					for (ulong aMapIdx = 0; aMapIdx < aFromVector.size(); aMapIdx++)
					{
						uint32_t aFromCp;
						uint32_t aToCp;
						if (TodUtf8SingleChar(aFromVector[aMapIdx], aFromCp) &&
							TodUtf8SingleChar(aToVector[aMapIdx], aToCp))
						{
							mCharMap[aFromCp] = aToCp;
						}
						else
							invalidParamFormat = true;
					}
				}
				else						
					sizeMismatch = true;						
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "CreateLayer") == 0)
	{
		if (theParams.mElementVector.size() == 2)
		{
			if (!theParams.mElementVector[1]->mIsList)
			{
				std::string aLayerName = StringToUpper(((SingleDataElement*) theParams.mElementVector[1])->mString);
				
				mFontLayerList.push_back(FontLayer(this));
				FontLayer* aFontLayer = &mFontLayerList.back();

				if (!mFontLayerMap.insert(FontLayerMap::value_type(aLayerName, aFontLayer)).second)
				{
					Error("Layer Already Exists");							
				}
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "CreateLayerFrom") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aSourceLayer;

			if ((!theParams.mElementVector[1]->mIsList) && (DataToLayer(theParams.mElementVector[2], &aSourceLayer)))
			{
				std::string aLayerName = StringToUpper(((SingleDataElement*) theParams.mElementVector[1])->mString);

				mFontLayerList.push_back(FontLayer(*aSourceLayer));
				FontLayer* aFontLayer = &mFontLayerList.back();

				if (!mFontLayerMap.insert(FontLayerMap::value_type(aLayerName, aFontLayer)).second)
				{
					Error("Layer Already Exists");							
				}
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerRequireTags") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			StringVector aStringVector;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(DataToStringVector(theParams.mElementVector[2], &aStringVector)))
			{
				for (ulong i = 0; i < aStringVector.size(); i++)
					aLayer->mRequiredTags.push_back(StringToUpper(aStringVector[i]));
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerExcludeTags") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			StringVector aStringVector;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(DataToStringVector(theParams.mElementVector[2], &aStringVector)))
			{
				for (ulong i = 0; i < aStringVector.size(); i++)
					aLayer->mExcludedTags.push_back(StringToUpper(aStringVector[i]));
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerPointRange") == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(!theParams.mElementVector[2]->mIsList) &&
				(!theParams.mElementVector[3]->mIsList))
			{
				int aMinPointSize;
				int aMaxPointSize;

				if ((StringToInt(((SingleDataElement*) theParams.mElementVector[2])->mString, &aMinPointSize)) &&
					(StringToInt(((SingleDataElement*) theParams.mElementVector[3])->mString, &aMaxPointSize)))
				{
					aLayer->mMinPointSize = aMinPointSize;
					aLayer->mMaxPointSize = aMaxPointSize;
				}
				else
					invalidParamFormat = true;						
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetPointSize") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(!theParams.mElementVector[2]->mIsList))
			{
				int aPointSize;
				if (StringToInt(((SingleDataElement*) theParams.mElementVector[2])->mString, &aPointSize))
				{
					aLayer->mPointSize = aPointSize;
				}
				else
					invalidParamFormat = true;						
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetHeight") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(!theParams.mElementVector[2]->mIsList))
			{
				int aHeight;
				if (StringToInt(((SingleDataElement*) theParams.mElementVector[2])->mString, &aHeight))
				{
					aLayer->mHeight = aHeight;
				}
				else
					invalidParamFormat = true;						
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetImage") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			std::string aFileNameString;
			
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(DataToString(theParams.mElementVector[2], &aFileNameString)))
			{						
				std::string aFileName = GetPathFrom(aFileNameString, GetFileDir(mSourceFile));
				
				bool isNew;
				SharedImageRef anImage = mApp->GetSharedImage(aFileName, "", &isNew);

				if ((Image*) anImage != NULL)
				{
					if (isNew)
						anImage->Palletize();
					aLayer->mImage = anImage;					
				}
				else
				{
					Error("Failed to Load Image");
					return false;
				}				
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetDrawMode") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && (!theParams.mElementVector[2]->mIsList))
			{						
				int anDrawMode;
				if ((StringToInt(((SingleDataElement*) theParams.mElementVector[2])->mString, &anDrawMode)) &&
					(anDrawMode >= 0) && (anDrawMode <= 1))
				{
					aLayer->mDrawMode = anDrawMode;
				}
				else
					invalidParamFormat = true;						
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetColorMult") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if (DataToLayer(theParams.mElementVector[1], &aLayer))
			{
				if (!GetColorFromDataElement(theParams.mElementVector[2],aLayer->mColorMult))
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetColorAdd") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if (DataToLayer(theParams.mElementVector[1], &aLayer))
			{
				if (!GetColorFromDataElement(theParams.mElementVector[2],aLayer->mColorAdd))
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetAscent") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(!theParams.mElementVector[2]->mIsList))
			{
				int anAscent;
				if (StringToInt(((SingleDataElement*) theParams.mElementVector[2])->mString, &anAscent))
				{
					aLayer->mAscent = anAscent;
				}
				else
					invalidParamFormat = true;						
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetAscentPadding") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(!theParams.mElementVector[2]->mIsList))
			{
				int anAscent;
				if (StringToInt(((SingleDataElement*) theParams.mElementVector[2])->mString, &anAscent))
				{
					aLayer->mAscentPadding = anAscent;
				}
				else
					invalidParamFormat = true;						
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetLineSpacingOffset") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(!theParams.mElementVector[2]->mIsList))
			{
				int anAscent;
				if (StringToInt(((SingleDataElement*) theParams.mElementVector[2])->mString, &anAscent))
				{
					aLayer->mLineSpacingOffset = anAscent;
				}
				else
					invalidParamFormat = true;						
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetOffset") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			IntVector anOffset;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && (DataToIntVector(theParams.mElementVector[2], &anOffset)) && (anOffset.size() == 2))
			{
				aLayer->mOffset.mX = anOffset[0];
				aLayer->mOffset.mY = anOffset[1];
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetCharWidths") == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			StringVector aCharsVector;
			IntVector aCharWidthsVector;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(DataToStringVector(theParams.mElementVector[2], &aCharsVector)) && 
				(DataToIntVector(theParams.mElementVector[3], &aCharWidthsVector)))
			{
				if (aCharsVector.size() == aCharWidthsVector.size())
				{
					for (ulong i = 0; i < aCharsVector.size(); i++)
					{
						uint32_t aCp;
						if (TodUtf8SingleChar(aCharsVector[i], aCp))
						{
							aLayer->mCharData[aCp].mWidth = aCharWidthsVector[i];
						}
						else
							invalidParamFormat = true;
					}
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetSpacing") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			IntVector anOffset;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(!theParams.mElementVector[2]->mIsList))
			{
				int aSpacing;

				if (StringToInt(((SingleDataElement*) theParams.mElementVector[2])->mString, &aSpacing))
				{
					aLayer->mSpacing = aSpacing;
				}
				else
					invalidParamFormat = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetImageMap") == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			StringVector aCharsVector;
			ListDataElement aRectList;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(DataToStringVector(theParams.mElementVector[2], &aCharsVector)) && 
				(DataToList(theParams.mElementVector[3], &aRectList)))
			{													
				if (aCharsVector.size() == aRectList.mElementVector.size())
				{
					if ((Image*) aLayer->mImage != NULL)
					{
						int anImageWidth = aLayer->mImage->GetWidth();
						int anImageHeight = aLayer->mImage->GetHeight();

						for (ulong i = 0; i < aCharsVector.size(); i++)
						{
							IntVector aRectElement;
							uint32_t aCp;

							if ((TodUtf8SingleChar(aCharsVector[i], aCp)) &&
								(DataToIntVector(aRectList.mElementVector[i], &aRectElement)) &&
								(aRectElement.size() == 4))
								
							{
								Rect aRect = Rect(aRectElement[0], aRectElement[1], aRectElement[2], aRectElement[3]);

								if ((aRect.mX < 0) || (aRect.mY < 0) || 
									(aRect.mX + aRect.mWidth > anImageWidth) || (aRect.mY + aRect.mHeight > anImageHeight))
								{
									Error("Image rectangle out of bounds");
									return false;
								}

								aLayer->mCharData[aCp].mImageRect = aRect;
							}
							else
								invalidParamFormat = true;
						}

													aLayer->mDefaultHeight = 0;
							for (std::map<uint32_t, CharData>::const_iterator aItr = aLayer->mCharData.begin();
								aItr != aLayer->mCharData.end(); ++aItr)
								if (aItr->second.mImageRect.mHeight + aItr->second.mOffset.mY > aLayer->mDefaultHeight)
									aLayer->mDefaultHeight = aItr->second.mImageRect.mHeight + aItr->second.mOffset.mY;
					}
					else
					{
						Error("Layer image not set");
						return false;
					}
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetCharOffsets") == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			StringVector aCharsVector;
			ListDataElement aRectList;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(DataToStringVector(theParams.mElementVector[2], &aCharsVector)) && 
				(DataToList(theParams.mElementVector[3], &aRectList)))
			{	
				if (aCharsVector.size() == aRectList.mElementVector.size())
				{
					for (ulong i = 0; i < aCharsVector.size(); i++)
					{
						IntVector aRectElement;
						uint32_t aCp;

						if ((TodUtf8SingleChar(aCharsVector[i], aCp)) &&
							(DataToIntVector(aRectList.mElementVector[i], &aRectElement)) &&
							(aRectElement.size() == 2))
						{
							aLayer->mCharData[aCp].mOffset = 
								Point(aRectElement[0], aRectElement[1]);
						}
						else
							invalidParamFormat = true;
					}							
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetKerningPairs") == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			StringVector aPairsVector;
			IntVector anOffsetsVector;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(DataToStringVector(theParams.mElementVector[2], &aPairsVector)) && 
				(DataToIntVector(theParams.mElementVector[3], &anOffsetsVector)))
			{
				if (aPairsVector.size() == anOffsetsVector.size())
				{
					for (ulong i = 0; i < aPairsVector.size(); i++)
					{
						if (aPairsVector[i].length() == 2)
						{
							aLayer->mCharData[(uchar) aPairsVector[i][0]].mKerningOffsets
								[(uchar) aPairsVector[i][1]] = anOffsetsVector[i];
						}
						else
							invalidParamFormat = true;
					}
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetBaseOrder") == 0)
	{
		if (theParams.mElementVector.size() == 3)
		{
			FontLayer* aLayer;
			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(!theParams.mElementVector[2]->mIsList))
			{
				int aBaseOrder;
				if (StringToInt(((SingleDataElement*) theParams.mElementVector[2])->mString, &aBaseOrder))
				{
					aLayer->mBaseOrder = aBaseOrder;
				}
				else
					invalidParamFormat = true;						
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetCharOrders") == 0)
	{
		if (theParams.mElementVector.size() == 4)
		{
			FontLayer* aLayer;
			StringVector aCharsVector;
			IntVector aCharOrdersVector;

			if ((DataToLayer(theParams.mElementVector[1], &aLayer)) && 
				(DataToStringVector(theParams.mElementVector[2], &aCharsVector)) && 
				(DataToIntVector(theParams.mElementVector[3], &aCharOrdersVector)))
			{
				if (aCharsVector.size() == aCharOrdersVector.size())
				{
					for (ulong i = 0; i < aCharsVector.size(); i++)
					{
						uint32_t aCp;
						if (TodUtf8SingleChar(aCharsVector[i], aCp))
						{
							aLayer->mCharData[aCp].mOrder = 
								aCharOrdersVector[i];
						}
						else
							invalidParamFormat = true;
					}
				}
				else
					sizeMismatch = true;
			}
			else
				invalidParamFormat = true;
		}
		else
			invalidNumParams = true;
	}
	else if (stricmp(aCmd.c_str(), "LayerSetExInfo") == 0)
	{
		// 中文年度版字体描述含扩展信息命令（ExInfo 仅元数据，字形渲染不使用），安全忽略
		if (theParams.mElementVector.size() != 4)
			invalidNumParams = true;
		else
		{
			FontLayer* aLayer;
			if (!DataToLayer(theParams.mElementVector[1], &aLayer))
				invalidParamFormat = true;
			// ExInfoKeys0 / ExInfoValues0 不参与字形渲染，直接忽略
		}
	}
	else 
	{
		Error("Unknown Command");
		return false;
	}

	if (invalidNumParams)
	{
		Error("Invalid Number of Parameters");
		return false;
	}

	if (invalidParamFormat)
	{
		Error("Invalid Paramater Type");
		return false;
	}

	if (literalError)
	{
		Error("Undefined Value");
		return false;
	}

	if (sizeMismatch)
	{
		Error("List Size Mismatch");
		return false;
	}

	return true;
}

bool FontData::Load(SexyAppBase* theSexyApp, const std::string& theFontDescFileName)
{
	if (mInitialized)
		return false;

	mApp = theSexyApp;
	mCurrentLine = "";

	mFontErrorHeader = "Font Descriptor Error in " + theFontDescFileName + "\r\n";
	
	mSourceFile = theFontDescFileName;	

	// 豆包修复：正确传播加载失败状态（原实现中 hasErrors 从未被赋值，
	// 即使 LoadDescriptor 失败也会返回 true，错误被上层吞掉）
	mInitialized = LoadDescriptor(theFontDescFileName);

	return mInitialized;
}

bool FontData::LoadLegacy(Image* theFontImage, const std::string& theFontDescFileName)
{
	if (mInitialized)
		return false;

	mFontLayerList.push_back(FontLayer(this));
	FontLayer* aFontLayer = &mFontLayerList.back();

	FontLayerMap::iterator anItr = mFontLayerMap.insert(FontLayerMap::value_type("", aFontLayer)).first;
	if (anItr == mFontLayerMap.end())
		return false;	
	
	aFontLayer->mImage = (SDL3Image*) theFontImage;	
	aFontLayer->mDefaultHeight = aFontLayer->mImage->GetHeight();	
	aFontLayer->mAscent = aFontLayer->mImage->GetHeight();	

	int aCharPos = 0;
	FILE *aStream = fopen(theFontDescFileName.c_str(), "r");

	if (aStream==NULL)
 		return false;

	mSourceFile = theFontDescFileName;

	int aSpaceWidth = 0;
	fscanf(aStream,"%d%d",&aFontLayer->mCharData[' '].mWidth,&aFontLayer->mAscent);
 
	while (!feof(aStream))
 	{
		char aBuf[2] = { 0, 0 }; // needed because fscanf will null terminate the string it reads
 		char aChar = 0;
 		int aWidth = 0;

		fscanf(aStream,"%1s%d",aBuf,&aWidth);
		aChar = aBuf[0];


		if (aChar == 0)
			break;

		aFontLayer->mCharData[aChar].mImageRect = Rect(aCharPos, 0, aWidth, aFontLayer->mImage->GetHeight());
		aFontLayer->mCharData[aChar].mWidth = aWidth;

		aCharPos += aWidth;
	}

	char c;
	
	for (c = 'A'; c <= 'Z'; c++)
		if ((aFontLayer->mCharData[c].mWidth == 0) && (aFontLayer->mCharData[c - 'A' + 'a'].mWidth != 0))
			mCharMap[c] = c - 'A' + 'a';

	for (c = 'a'; c <= 'z'; c++)
		if ((aFontLayer->mCharData[c].mWidth == 0) && (aFontLayer->mCharData[c - 'a' + 'A'].mWidth != 0))
			mCharMap[c] = c - 'a' + 'A';

	mInitialized = true;
	fclose(aStream);

	return true;
}

////

ActiveFontLayer::ActiveFontLayer()
{
	mScaledImage = NULL;
	mOwnsImage = false;
}

ActiveFontLayer::ActiveFontLayer(const ActiveFontLayer& theActiveFontLayer) : 
	mBaseFontLayer(theActiveFontLayer.mBaseFontLayer),
	mScaledImage(theActiveFontLayer.mScaledImage),
	mOwnsImage(theActiveFontLayer.mOwnsImage)
{
	if (mOwnsImage)	
		mScaledImage = mBaseFontLayer->mFontData->mApp->CopyImage(mScaledImage);	

	mScaledCharImageRects = theActiveFontLayer.mScaledCharImageRects;
}

ActiveFontLayer::~ActiveFontLayer()
{
	if (mOwnsImage)
		delete mScaledImage;
}

////

ImageFont::ImageFont(SexyAppBase* theSexyApp, const std::string& theFontDescFileName)
{	
	mScale = 1.0;
	mFontData = new FontData();
	mFontData->Ref();
	mFontData->Load(theSexyApp, theFontDescFileName);
	mPointSize = mFontData->mDefaultPointSize;
	GenerateActiveFontLayers();
	mActiveListValid = true;
	mForceScaledImagesWhite = false;
}

ImageFont::ImageFont(Image *theFontImage)
{
	mScale = 1.0;
	mFontData = new FontData();
	mFontData->Ref();
	mFontData->mInitialized = true;
	mPointSize = mFontData->mDefaultPointSize;
	mActiveListValid = false;
	mForceScaledImagesWhite = false;

	mFontData->mFontLayerList.push_back(FontLayer(mFontData));
	FontLayer* aFontLayer = &mFontData->mFontLayerList.back();

	mFontData->mFontLayerMap.insert(FontLayerMap::value_type("", aFontLayer)).first;	
	aFontLayer->mImage = (SDL3Image*) theFontImage;	
	aFontLayer->mDefaultHeight = aFontLayer->mImage->GetHeight();	
	aFontLayer->mAscent = aFontLayer->mImage->GetHeight();	
}

ImageFont::ImageFont(const ImageFont& theImageFont) :
	Font(theImageFont),
	mScale(theImageFont.mScale),
	mFontData(theImageFont.mFontData),
	mPointSize(theImageFont.mPointSize),	
	mTagVector(theImageFont.mTagVector),
	mActiveListValid(theImageFont.mActiveListValid),
	mForceScaledImagesWhite(theImageFont.mForceScaledImagesWhite)
{
	mFontData->Ref();	
	
	if (mActiveListValid)
		mActiveLayerList = theImageFont.mActiveLayerList;	
}

ImageFont::ImageFont(Image* theFontImage, const std::string& theFontDescFileName)
{
	
	mScale = 1.0;
	mFontData = new FontData();
	mFontData->Ref();
	mFontData->LoadLegacy(theFontImage, theFontDescFileName);	
	mPointSize = mFontData->mDefaultPointSize;
	GenerateActiveFontLayers();
	mActiveListValid = true;
}

ImageFont::~ImageFont()
{
	mFontData->DeRef();
}

/*ImageFont::ImageFont(const ImageFont& theImageFont, Image* theImage) :
	Font(theImageFont),
	mImage(theImage)
{
	for (int i = 0; i < 256; i++)
	{
		mCharPos[i] = theImageFont.mCharPos[i];
		mCharWidth[i] = theImageFont.mCharWidth[i];
	}
}*/

void ImageFont::GenerateActiveFontLayers()
{
	if (!mFontData->mInitialized)
		return;

	mActiveLayerList.clear();

	ulong i;

	mAscent = 0;	
	mAscentPadding = 0;
	mHeight = 0;	
	mLineSpacingOffset = 0;

	FontLayerList::iterator anItr = mFontData->mFontLayerList.begin();

	bool firstLayer = true;

	while (anItr != mFontData->mFontLayerList.end())
	{
		FontLayer* aFontLayer = &*anItr;		

		if ((mPointSize >= aFontLayer->mMinPointSize) &&
			((mPointSize <= aFontLayer->mMaxPointSize) || (aFontLayer->mMaxPointSize == -1)))
		{
			bool active = true;

			// Make sure all required tags are included
			for (i = 0; i < aFontLayer->mRequiredTags.size(); i++)
				if (std::find(mTagVector.begin(), mTagVector.end(), aFontLayer->mRequiredTags[i]) == mTagVector.end())
					active = false;

			// Make sure no excluded tags are included
			for (i = 0; i < mTagVector.size(); i++)
				if (std::find(aFontLayer->mExcludedTags.begin(), aFontLayer->mExcludedTags.end(), 
					mTagVector[i]) != aFontLayer->mExcludedTags.end())
					active = false;

			if (active)
			{				
				mActiveLayerList.push_back(ActiveFontLayer());

				ActiveFontLayer* anActiveFontLayer = &mActiveLayerList.back();

				anActiveFontLayer->mBaseFontLayer = aFontLayer;

				double aLayerPointSize = 1;
				double aPointSize = mScale;
				
				if ((mScale == 1.0) && ((aFontLayer->mPointSize == 0) || (mPointSize == aFontLayer->mPointSize)))
				{
					anActiveFontLayer->mScaledImage = aFontLayer->mImage;
					anActiveFontLayer->mOwnsImage = false;
					
					// Use the specified point size
					
					for (std::map<uint32_t, CharData>::const_iterator aItr = aFontLayer->mCharData.begin();
							aItr != aFontLayer->mCharData.end(); ++aItr)
							anActiveFontLayer->mScaledCharImageRects[aItr->first] = aItr->second.mImageRect;
				}
				else
				{				
					if (aFontLayer->mPointSize != 0)
					{
						aLayerPointSize = aFontLayer->mPointSize;
						aPointSize = mPointSize * mScale;
					}

					// Resize font elements
SDL3Image* aMemoryImage = new SDL3Image(LawnApp::mSDLRenderer);
					
					int aCurX = 0;
					int aMaxHeight = 0;
					
					for (std::map<uint32_t, CharData>::const_iterator aItr = aFontLayer->mCharData.begin();
						aItr != aFontLayer->mCharData.end(); ++aItr)
					{
						uint32_t aCharNum = aItr->first;
						Rect* anOrigRect = &aFontLayer->mCharData[aCharNum].mImageRect;

						Rect aScaledRect(aCurX, 0,  
							(int) ((anOrigRect->mWidth * aPointSize) / aLayerPointSize),
							(int) ((anOrigRect->mHeight * aPointSize) / aLayerPointSize));

						anActiveFontLayer->mScaledCharImageRects[aCharNum] = aScaledRect;

						if (aScaledRect.mHeight > aMaxHeight)
							aMaxHeight = aScaledRect.mHeight;

						aCurX += aScaledRect.mWidth;
					}
										
					anActiveFontLayer->mScaledImage = aMemoryImage;
					anActiveFontLayer->mOwnsImage = true;
					
					// Create the image now

					aMemoryImage->mWidth = aCurX;
					aMemoryImage->mHeight = aMaxHeight;
					aMemoryImage->mD3DData = SDL_CreateTexture(LawnApp::mSDLRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, aMemoryImage->mWidth, aMemoryImage->mHeight);
					SDL_SetTextureBlendMode((SDL_Texture*)aMemoryImage->mD3DData, SDL_BLENDMODE_BLEND);
					Graphics g(aMemoryImage);
					g.SetLinearBlend(true);

					SDL_SetRenderTarget(LawnApp::mSDLRenderer, (SDL_Texture*)aMemoryImage->mD3DData);

					for (std::map<uint32_t, CharData>::const_iterator aItr = aFontLayer->mCharData.begin();
						aItr != aFontLayer->mCharData.end(); ++aItr)
					{
						uint32_t aCharNum = aItr->first;
						if ((Image*) aFontLayer->mImage != NULL)
							g.DrawImage(aFontLayer->mImage, anActiveFontLayer->mScaledCharImageRects[aCharNum],
								aFontLayer->mCharData[aCharNum].mImageRect);						
					}

					if (mForceScaledImagesWhite)
					{
						SDL_SetRenderTarget(LawnApp::mSDLRenderer, (SDL_Texture*)aMemoryImage->mD3DData);

						SDL_Surface* surface = SDL_RenderReadPixels(LawnApp::mSDLRenderer, nullptr);
						Uint32* pixels = (Uint32*)surface->pixels;
						int width = surface->w;
						int height = surface->h;
						int pitch = surface->pitch / 4; 

						for (int y = 0; y < height; y++)
						{
							Uint32* row = pixels + y * pitch;
							for (int x = 0; x < width; x++)
							{
								Uint32 pixel = row[x];
								row[x] = (pixel & 0xFF000000) | 0x00FFFFFF; 
							}
						}

						SDL_UpdateTexture((SDL_Texture*)aMemoryImage->mD3DData, nullptr, surface->pixels, surface->pitch);
						SDL_DestroySurface(surface);
						SDL_SetRenderTarget(LawnApp::mSDLRenderer, nullptr);
					}

					SDL_SetRenderTarget(LawnApp::mSDLRenderer, nullptr);
				}

				int aLayerAscent = (aFontLayer->mAscent * aPointSize) / aLayerPointSize;
				if (aLayerAscent > mAscent)
					mAscent = aLayerAscent;

				if (aFontLayer->mHeight != 0)
				{
					int aLayerHeight = (aFontLayer->mHeight * aPointSize) / aLayerPointSize;
					if (aLayerHeight > mHeight)
						mHeight = aLayerHeight;
				}
				else
				{
					int aLayerHeight = (aFontLayer->mDefaultHeight * aPointSize) / aLayerPointSize;
					if (aLayerHeight > mHeight)
						mHeight = aLayerHeight;
				}

				int anAscentPadding = (aFontLayer->mAscentPadding * aPointSize) / aLayerPointSize;
				if ((firstLayer) || (anAscentPadding < mAscentPadding))
					mAscentPadding = anAscentPadding;

				int aLineSpacingOffset = (aFontLayer->mLineSpacingOffset * aPointSize) / aLayerPointSize;
				if ((firstLayer) || (aLineSpacingOffset > mLineSpacingOffset))
					mLineSpacingOffset = aLineSpacingOffset;

				firstLayer = false;
			}
		}

		++anItr;
	}	
}

int ImageFont::StringWidth(const SexyString& theString)
{
	int aWidth = 0;
	uint32_t aPrevChar = 0;
	size_t aPos = 0;
	while (aPos < theString.length())
	{
		uint32_t aChar = Sexy::Utf8Decode(theString, aPos);
		aWidth += CharWidthKern(aChar, aPrevChar);
		aPrevChar = aChar;
	}

	return aWidth;
}

int ImageFont::CharWidthKern(uint32_t theChar, uint32_t thePrevChar)
{
	Prepare();

	int aMaxXPos = 0;
	double aPointSize = mPointSize * mScale;

	theChar = mFontData->MapChar(theChar);
	if (thePrevChar != 0)
		thePrevChar = mFontData->MapChar(thePrevChar);

	ActiveFontLayerList::iterator anItr = mActiveLayerList.begin();
	while (anItr != mActiveLayerList.end())
	{
		ActiveFontLayer* anActiveFontLayer = &*anItr;
		
		int aLayerXPos = 0;
		
		int aCharWidth;
		int aSpacing;

		int aLayerPointSize = anActiveFontLayer->mBaseFontLayer->mPointSize;

		if (aLayerPointSize == 0)
		{
			aCharWidth = anActiveFontLayer->mBaseFontLayer->mCharData[theChar].mWidth * mScale;

			if (thePrevChar != 0)
			{
				aSpacing = (anActiveFontLayer->mBaseFontLayer->mSpacing + 
					anActiveFontLayer->mBaseFontLayer->mCharData[thePrevChar].GetKerningOffset(theChar)) * mScale;
			}
			else
				aSpacing = 0;
		}
		else
		{
			aCharWidth = (anActiveFontLayer->mBaseFontLayer->mCharData[theChar].mWidth * aPointSize / aLayerPointSize);
			
			if (thePrevChar != 0)
			{
				aSpacing = (anActiveFontLayer->mBaseFontLayer->mSpacing + 
					anActiveFontLayer->mBaseFontLayer->mCharData[thePrevChar].GetKerningOffset(theChar)) * aPointSize / aLayerPointSize;
			}
			else
				aSpacing = 0;
		}						
		
		aLayerXPos += aCharWidth + aSpacing;

		if (aLayerXPos > aMaxXPos)
			aMaxXPos = aLayerXPos;

		++anItr;
	}

	return aMaxXPos;
}

int ImageFont::CharWidth(uint32_t theChar)
{
	return CharWidthKern(theChar,0);
}

CritSect gRenderCritSec;
static const int POOL_SIZE = 4096;
static RenderCommand gRenderCommandPool[POOL_SIZE];
static RenderCommand* gRenderTail[256];
static RenderCommand* gRenderHead[256];

void ImageFont::DrawStringEx(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect* theClipRect, RectList* theDrawnAreas, int* theWidth)
{
	AutoCrit anAutoCrit(gRenderCritSec);

	int aPoolIdx;

	for (aPoolIdx = 0; aPoolIdx < 256; aPoolIdx++)
	{
		gRenderHead[aPoolIdx] = NULL;
		gRenderTail[aPoolIdx] = NULL;
	}

	int aXPos = theX;	

	if (theDrawnAreas != NULL)
		theDrawnAreas->clear();
	

	/*if (theDrawnArea != NULL)
		*theDrawnArea = Rect(0, 0, 0, 0);*/

	if (!mFontData->mInitialized)
	{
		if (theWidth != NULL)
			*theWidth = 0;
		return;
	}
	
	Prepare();

	bool colorizeImages = g->GetColorizeImages();
	g->SetColorizeImages(true);	

	int aCurXPos = theX;
	int aCurPoolIdx = 0;

	std::vector<uint32_t> aCps;
	Sexy::Utf8ToCodePoints(theString, aCps);

	for (ulong aCharNum = 0; aCharNum < aCps.size(); aCharNum++)
	{
		uint32_t aChar = mFontData->MapChar(aCps[aCharNum]);
		
		uint32_t aNextChar = 0;
		if (aCharNum < aCps.size() - 1)
			aNextChar = mFontData->MapChar(aCps[aCharNum+1]);

		int aMaxXPos = aCurXPos;
		
		int aLayerCount = 0;

		ActiveFontLayerList::iterator anItr = mActiveLayerList.begin();
		while (anItr != mActiveLayerList.end())
		{
			ActiveFontLayer* anActiveFontLayer = &*anItr;
			
			int aLayerXPos = aCurXPos;
			
			int anImageX;
			int anImageY;
			int aCharWidth;
			int aSpacing;

			int aLayerPointSize = anActiveFontLayer->mBaseFontLayer->mPointSize;

			double aScale = mScale;
			if (aLayerPointSize != 0)
				aScale *= mPointSize / aLayerPointSize;

			if (aScale == 1.0)
			{
				anImageX = aLayerXPos + anActiveFontLayer->mBaseFontLayer->mOffset.mX + anActiveFontLayer->mBaseFontLayer->mCharData[aChar].mOffset.mX;
				anImageY = theY - (anActiveFontLayer->mBaseFontLayer->mAscent - anActiveFontLayer->mBaseFontLayer->mOffset.mY - anActiveFontLayer->mBaseFontLayer->mCharData[aChar].mOffset.mY);
				aCharWidth = anActiveFontLayer->mBaseFontLayer->mCharData[aChar].mWidth;				
				
				if (aNextChar != 0)
				{
					 aSpacing = anActiveFontLayer->mBaseFontLayer->mSpacing + 
						 anActiveFontLayer->mBaseFontLayer->mCharData[aChar].GetKerningOffset(aNextChar);
				}
				else
					aSpacing = 0;
			}
			else
			{
				anImageX = aLayerXPos + (int) ((anActiveFontLayer->mBaseFontLayer->mOffset.mX + anActiveFontLayer->mBaseFontLayer->mCharData[aChar].mOffset.mX) * aScale);
				anImageY = theY - (int) ((anActiveFontLayer->mBaseFontLayer->mAscent - anActiveFontLayer->mBaseFontLayer->mOffset.mY - anActiveFontLayer->mBaseFontLayer->mCharData[aChar].mOffset.mY) * aScale);
				aCharWidth = (anActiveFontLayer->mBaseFontLayer->mCharData[aChar].mWidth * aScale);
				
				if (aNextChar != 0)
				{
					 aSpacing = (int) ((anActiveFontLayer->mBaseFontLayer->mSpacing + 
						 anActiveFontLayer->mBaseFontLayer->mCharData[aChar].GetKerningOffset(aNextChar)) * aScale);
				}
				else
					aSpacing = 0;
			}						
			
			Color aColor;
			aColor.mRed = min((theColor.mRed * anActiveFontLayer->mBaseFontLayer->mColorMult.mRed / 255) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mRed, 255);
			aColor.mGreen = min((theColor.mGreen * anActiveFontLayer->mBaseFontLayer->mColorMult.mGreen / 255) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mGreen, 255);
			aColor.mBlue = min((theColor.mBlue * anActiveFontLayer->mBaseFontLayer->mColorMult.mBlue / 255) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mBlue, 255);
			aColor.mAlpha = min((theColor.mAlpha * anActiveFontLayer->mBaseFontLayer->mColorMult.mAlpha / 255) + anActiveFontLayer->mBaseFontLayer->mColorAdd.mAlpha, 255);
			
			int aBaseOrder = anActiveFontLayer->mBaseFontLayer->mBaseOrder;
			if (!aBaseOrder) aBaseOrder = aLayerCount++;
			int anOrder = aBaseOrder + anActiveFontLayer->mBaseFontLayer->mCharData[aChar].mOrder;

			if (aCurPoolIdx >= POOL_SIZE)
				break;

			RenderCommand* aRenderCommand = &gRenderCommandPool[aCurPoolIdx++];

			aRenderCommand->mImage = anActiveFontLayer->mScaledImage;
			aRenderCommand->mColor = aColor;
			aRenderCommand->mDest[0] = anImageX;
			aRenderCommand->mDest[1] = anImageY;
			aRenderCommand->mSrc[0] = anActiveFontLayer->mScaledCharImageRects[aChar].mX;
			aRenderCommand->mSrc[1] = anActiveFontLayer->mScaledCharImageRects[aChar].mY;
			aRenderCommand->mSrc[2] = anActiveFontLayer->mScaledCharImageRects[aChar].mWidth;
			aRenderCommand->mSrc[3] = anActiveFontLayer->mScaledCharImageRects[aChar].mHeight;
			aRenderCommand->mMode = anActiveFontLayer->mBaseFontLayer->mDrawMode;
			aRenderCommand->mNext = NULL;

			int anOrderIdx = min(max(anOrder + 128, 0), 255);

			if (gRenderTail[anOrderIdx] == NULL)
			{
				gRenderTail[anOrderIdx] = aRenderCommand;
				gRenderHead[anOrderIdx] = aRenderCommand;
			}
			else
			{
				gRenderTail[anOrderIdx]->mNext = aRenderCommand;
				gRenderTail[anOrderIdx] = aRenderCommand;
			}

			//aRenderCommandMap.insert(RenderCommandMap::value_type(aPriority, aRenderCommand));

			/*int anOldDrawMode = g->GetDrawMode();
			if (anActiveFontLayer->mBaseFontLayer->mDrawMode != -1)
				g->SetDrawMode(anActiveFontLayer->mBaseFontLayer->mDrawMode);
			Color anOrigColor = g->GetColor();
			g->SetColor(aColor);			
			if (anActiveFontLayer->mScaledImage != NULL)
				g->DrawImage(anActiveFontLayer->mScaledImage, anImageX, anImageY, anActiveFontLayer->mScaledCharImageRects[aChar]);	
			g->SetColor(anOrigColor);
			g->SetDrawMode(anOldDrawMode);*/

			if (theDrawnAreas != NULL)
			{
				Rect aDestRect = Rect(anImageX, anImageY, anActiveFontLayer->mScaledCharImageRects[aChar].mWidth, anActiveFontLayer->mScaledCharImageRects[aChar].mHeight);

				theDrawnAreas->push_back(aDestRect);

				/*if (theDrawnArea->mWidth == 0)
					*theDrawnArea = theDestRect;
				else
				{
					if (theDestRect.mX < theDrawnArea->mX)
					{
						int aDiff = theDestRect.mX - theDrawnArea->mX;
						theDrawnArea->mX += aDiff;
						theDrawnArea->mWidth += aDiff;
					}
					
					if (theDestRect.mX + theDestRect.mWidth > theDrawnArea->mX + theDrawnArea->mWidth)
						theDrawnArea->mWidth = theDestRect.mX + theDestRect.mWidth - theDrawnArea->mX;

					if (theDestRect.mY < theDrawnArea->mY)
					{
						int aDiff = theDestRect.mY - theDrawnArea->mY;
						theDrawnArea->mY += aDiff;
						theDrawnArea->mHeight += aDiff;
					}
					
					if (theDestRect.mY + theDestRect.mHeight > theDrawnArea->mY + theDrawnArea->mHeight)
						theDrawnArea->mHeight = theDestRect.mY + theDestRect.mHeight - theDrawnArea->mY;
				}*/
			}

			aLayerXPos += aCharWidth + aSpacing;

			if (aLayerXPos > aMaxXPos)
				aMaxXPos = aLayerXPos;

			++anItr;
		}

		aCurXPos = aMaxXPos;
	}

	if (theWidth != NULL)
		*theWidth = aCurXPos - theX;

	Color anOrigColor = g->GetColor();

	for (aPoolIdx = 0; aPoolIdx < 256; aPoolIdx++)
	{		
		RenderCommand* aRenderCommand = gRenderHead[aPoolIdx];

		while (aRenderCommand != NULL)
		{
			int anOldDrawMode = g->GetDrawMode();
			if (aRenderCommand->mMode != -1)
				g->SetDrawMode(aRenderCommand->mMode);			
			g->SetColor(Color(aRenderCommand->mColor));
			if (aRenderCommand->mImage != NULL)
				g->DrawImage(aRenderCommand->mImage, aRenderCommand->mDest[0], aRenderCommand->mDest[1], Rect(aRenderCommand->mSrc[0], aRenderCommand->mSrc[1], aRenderCommand->mSrc[2], aRenderCommand->mSrc[3]));				
			g->SetDrawMode(anOldDrawMode);

			aRenderCommand = aRenderCommand->mNext;
		}
	}

	g->SetColor(anOrigColor);
	
	/*RenderCommandMap::iterator anItr = aRenderCommandMap.begin();
	while (anItr != aRenderCommandMap.end())
	{
		RenderCommand* aRenderCommand = &anItr->second;

		int anOldDrawMode = g->GetDrawMode();
		if (aRenderCommand->mMode != -1)
			g->SetDrawMode(aRenderCommand->mMode);
		Color anOrigColor = g->GetColor();
		g->SetColor(aRenderCommand->mColor);			
		if (aRenderCommand->mImage != NULL)
			g->DrawImage(aRenderCommand->mImage, aRenderCommand->mDest.mX, aRenderCommand->mDest.mY, aRenderCommand->mSrc);	
		g->SetColor(anOrigColor);
		g->SetDrawMode(anOldDrawMode);

		++anItr;
	}*/		

	g->SetColorizeImages(colorizeImages);
}

uint32_t ImageFont::GetMappedChar(uint32_t value)
{
	return mFontData->MapChar(value);
}

void ImageFont::DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& theClipRect)
{
	DrawStringEx(g, theX, theY, theString, theColor, &theClipRect, NULL, NULL);	
}

Font* ImageFont::Duplicate()
{
	return new ImageFont(*this);
}

void ImageFont::SetPointSize(int thePointSize)
{
	mPointSize = thePointSize;
	mActiveListValid = false;
}

void ImageFont::SetScale(double theScale)
{
	mScale = theScale;
	mActiveListValid = false;
}

int	ImageFont::GetPointSize()
{
	return mPointSize;
}

int	ImageFont::GetDefaultPointSize()
{
	return mFontData->mDefaultPointSize;
}

bool ImageFont::AddTag(const std::string& theTagName)
{
	if (HasTag(theTagName))
		return false;

	std::string aTagName = StringToUpper(theTagName);
	mTagVector.push_back(aTagName);
	mActiveListValid = false;
	return true;
}

bool ImageFont::RemoveTag(const std::string& theTagName)
{
	std::string aTagName = StringToUpper(theTagName);

	StringVector::iterator anItr = std::find(mTagVector.begin(), mTagVector.end(), aTagName);
	if (anItr == mTagVector.end())
		return false;

	mTagVector.erase(anItr);
	mActiveListValid = false;
	return true;
}

bool ImageFont::HasTag(const std::string& theTagName)
{
	StringVector::iterator anItr = std::find(mTagVector.begin(), mTagVector.end(), theTagName);
	return anItr != mTagVector.end();
}

std::string ImageFont::GetDefine(const std::string& theName)
{
	DataElement* aDataElement = mFontData->Dereference(theName);

	if (aDataElement == NULL)
		return "";

	return mFontData->DataElementToString(aDataElement);
}

void ImageFont::Prepare()
{
	if (!mActiveListValid)
	{
		GenerateActiveFontLayers();
		mActiveListValid = true;
	}
}
