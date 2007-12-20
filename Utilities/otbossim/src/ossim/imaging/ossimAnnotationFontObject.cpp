//*******************************************************************
// Copyright (C) 2000 ImageLinks Inc. 
//
// License:  See top level LICENSE.txt file.
//
// Author: Garrett Potts
//
//*************************************************************************
// $Id: ossimAnnotationFontObject.cpp 11347 2007-07-23 13:01:59Z gpotts $

#include <ossim/imaging/ossimAnnotationFontObject.h>
#include <ossim/font/ossimFontFactoryRegistry.h>

RTTI_DEF1(ossimAnnotationFontObject, "ossimAnnotationFontObject", ossimAnnotationObject);

ossimAnnotationFontObject::ossimAnnotationFontObject()
   :ossimAnnotationObject(),
    theFont(ossimFontFactoryRegistry::instance()->getDefaultFont()),
    theOwnsFontFlag(false),
    theCenterPosition(0,0),
    theString(""),
    theRotation(0.0),
    theHorizontalScale(0.0),
    theVerticalScale(0.0),
    theHorizontalShear(0.0),
    theVerticalShear(0.0)
{
   setFontInfo();
   theBoundingRect.makeNan();
}

ossimAnnotationFontObject::ossimAnnotationFontObject(const ossimIpt& upperLeft,
                                                     const ossimString& s,
                                                     const ossimIpt& pixelSize,
                                                     double rotation,
                                                     const ossimDpt& scale,
                                                     const ossimDpt& shear,
                                                     unsigned char r,
                                                     unsigned char g,
                                                     unsigned char b)
   :ossimAnnotationObject(r,g,b),
    theFont(ossimFontFactoryRegistry::instance()->getDefaultFont()),
    theOwnsFontFlag(false),
    theString(s),
    thePixelSize(pixelSize),
    theRotation(rotation),
    theHorizontalScale(scale.x),
    theVerticalScale(scale.y),
    theHorizontalShear(shear.x),
    theVerticalShear(shear.y)
{
   setFontInfo();
   if (theFont)
   {
      theFont->getBoundingBox(theBoundingRect);
   }
   theBoundingRect += upperLeft;
   theCenterPosition = theBoundingRect.midPoint();
}

ossimAnnotationFontObject::ossimAnnotationFontObject(const ossimAnnotationFontObject& rhs)
   :ossimAnnotationObject(rhs),
    theCenterPosition(rhs.theCenterPosition),
    theString(rhs.theString),
    thePixelSize(rhs.thePixelSize),
    theRotation(rhs.theRotation),
    theHorizontalScale(rhs.theHorizontalScale),
    theVerticalScale(rhs.theVerticalScale),
    theHorizontalShear(rhs.theHorizontalShear),
    theVerticalShear(rhs.theVerticalShear),
    theBoundingRect(rhs.theBoundingRect)
{
   if(rhs.theOwnsFontFlag&&theFont)
   {
      theFont = (ossimFont*)rhs.theFont->dup();
   }
   else
   {
      theFont = rhs.theFont;
   }
   
   setFontInfo();
}


ossimAnnotationFontObject::~ossimAnnotationFontObject()
{
   if(theFont&&theOwnsFontFlag)
   {
      delete theFont;
      theFont = NULL;
      theOwnsFontFlag = false;
   }
   else
   {
      theFont = NULL;
      theOwnsFontFlag = false;
   }
}

void ossimAnnotationFontObject::draw(ossimRgbImage& anImage)const
{
   if(theBoundingRect.hasNans())
   {
      return;
   }
   if (!theFont)
   {
      return;
   }
   
   if(!thePixelSize.x || !thePixelSize.y)
   {
      return; // (DAH) we have to have some demension to write!
   }

   ossimRefPtr<ossimImageData> destination = anImage.getImageData();
   
   if(destination.valid())
   {
      ossimIrect destRect = anImage.getImageData()->getImageRectangle();
      
      ossimIpt shift(-theBoundingRect.ul().x,
                     -theBoundingRect.ul().y);

      destRect += shift;
      ossimIrect boundingRect = theBoundingRect + shift;
      
      if(boundingRect.intersects(destRect))
      {
         setFontInfo();
 
         ossimIrect fontBufferRect;
         ossimIrect boundingFontBox;
         
         theFont->setClippingBox(destRect);
         theFont->getBoundingBox(boundingFontBox);

         const ossim_uint8* srcBuf = theFont->rasterize();
         if (!srcBuf)
         {
            return;
         }
         
         theFont->getBufferRect(fontBufferRect);

         ossimIrect clipRect = boundingRect.clipToRect(fontBufferRect);
         
         long clipHeight = clipRect.height();
         long clipWidth  = clipRect.width();
         
         long destinationOffset   = (long)(((clipRect.ul().y - destRect.ul().y)* destRect.width()) +
                                           (clipRect.ul().x - destRect.ul().x));
         long fontBufferOffset    = (long)(((clipRect.ul().y - fontBufferRect.ul().y)*fontBufferRect.width()) +
                                           (clipRect.ul().x - fontBufferRect.ul().x));

         
         long destinationIndex = destinationOffset;
         long srcIndex         = fontBufferOffset;
         long num_bands        = destination->getNumberOfBands();
         ossim_int32 s_width = (ossim_int32)fontBufferRect.width();
         ossim_int32 d_width = (ossim_int32)destRect.width();
         
         num_bands = num_bands > 3? 3:num_bands;
         
         unsigned char colorArray[3];
         long band = 0;
         colorArray[0] = theRed;
         colorArray[1] = theGreen;
         colorArray[2] = theBlue;
         ossim_uint8** destBuf = new ossim_uint8*[num_bands];
         for(band = 0; band < num_bands;++band)
         {
            destBuf[band] = static_cast<ossim_uint8*>(destination->getBuf(band));
            if (!destBuf[band])
            {
               delete [] destBuf;
               return;
            }
         }
         
         for(long line = 0; line < clipHeight; ++line)
         {
            for(long col = 0; col < clipWidth; ++col)
            {
               if(srcBuf[srcIndex + col])
               {
                  for (band=0; band<num_bands; ++band)
                  {
                     *(destBuf[band] + destinationIndex + col) = colorArray[band];
                  }
               }
            }
            srcIndex         += s_width;
            destinationIndex += d_width;
         }
         delete [] destBuf;
      }
   }
}

std::ostream& ossimAnnotationFontObject::print(std::ostream& out)const
{
   if(theFont)
   {
      out << "Family:          " << theFont->getFamilyName() << endl
          << "Style:           " << theFont->getStyleName()  << endl;
   }
   out << "String:             " << theString << endl
       << "Center Position:    " << theCenterPosition << endl
       << "Rotation:           " << theRotation << endl
       << "Horizontal shear:   " << theHorizontalShear << endl
       << "Vertical shear:     " << theVerticalShear << endl
       << "Vertical scale:     " << theVerticalScale << endl
       << "Horizontal scale:   " << theHorizontalScale << endl
       << "Bounding rect:      " << theBoundingRect << endl;
   return out;
}

void ossimAnnotationFontObject::getBoundingRect(ossimDrect& rect)const
{
   rect =  theBoundingRect;
}

void ossimAnnotationFontObject::computeBoundingRect()
{
   setFontInfo();
   if(theFont)
   {
      ossimIrect textRect;
      theFont->getBoundingBox(textRect);
      ossim_int32 w = textRect.width();
      ossim_int32 h = textRect.height();
      ossim_int32 ulx = theCenterPosition.x - w/2;
      ossim_int32 uly = theCenterPosition.y - h/2;
      theBoundingRect = ossimIrect(ulx,
                                   uly,
                                   ulx + w - 1,
                                   uly + h - 1);
   }
}

bool ossimAnnotationFontObject::isPointWithin(const ossimDpt& imagePoint)const
{
   return theBoundingRect.pointWithin(imagePoint);
}

void ossimAnnotationFontObject::setFont(ossimFont* font,
                                        bool ownsFontFlag)
{
   if(theOwnsFontFlag)
   {
      delete theFont;
      theFont = (ossimFont*)NULL;
   }
   theFont = font;
   theOwnsFontFlag = ownsFontFlag;

   if(!theFont)
   {
      theFont = ossimFontFactoryRegistry::instance()->getDefaultFont();
      theOwnsFontFlag = false;
   }
}

void ossimAnnotationFontObject::setPositionCenter(const ossimIpt& position)
{
   theCenterPosition = position;
   computeBoundingRect();
}

void ossimAnnotationFontObject::setFontInfo()const
{
   if(theFont)
   {
      theFont->setString(theString);
      theFont->setRotation(theRotation);
      theFont->setScale(theHorizontalScale, theVerticalScale);
      theFont->setHorizontalVerticalShear(theHorizontalShear, theVerticalShear);
      theFont->setPixelSize(thePixelSize.x,
                            thePixelSize.y);
   }
}

void ossimAnnotationFontObject::setString(const ossimString& s)
{
   theString = s;
}

ossimString ossimAnnotationFontObject::getString()const
{
   return theString;
}

ossimAnnotationObject* ossimAnnotationFontObject::getNewClippedObject(const ossimDrect& rect)const
{
   if(intersects(rect))
   {
      return (ossimAnnotationObject*)dup();
   }
   
   return (ossimAnnotationObject*)NULL;
}

bool ossimAnnotationFontObject::intersects(const ossimDrect& rect)const
{
   return rect.intersects(theBoundingRect);
}

void ossimAnnotationFontObject::setPointSize(const ossimIpt& size)
{
   thePixelSize = size;
   setFontInfo();
   if (theFont)
   {
      theFont->getBoundingBox(theBoundingRect);
   }
   theBoundingRect += (theCenterPosition - theBoundingRect.midPoint());
   theCenterPosition = theBoundingRect.midPoint();
}

void ossimAnnotationFontObject::setRotation(double rotation)
{
   theRotation = rotation;
   setFontInfo();
   if (theFont)
   {
      theFont->getBoundingBox(theBoundingRect);
   }
   theBoundingRect += (theCenterPosition - theBoundingRect.midPoint());
   theCenterPosition = theBoundingRect.midPoint();         
}

void ossimAnnotationFontObject::setScale(const ossimDpt& scale)
{
   theHorizontalScale = scale.x;
   theVerticalScale   = scale.y;
   setFontInfo();
   if (theFont)
   {
      theFont->getBoundingBox(theBoundingRect);
   }
   theBoundingRect += (theCenterPosition - theBoundingRect.midPoint());
   theCenterPosition = theBoundingRect.midPoint();         
}

void ossimAnnotationFontObject::setShear(const ossimDpt& shear)
{
   theHorizontalShear = shear.x;
   theVerticalShear   = shear.y;
   setFontInfo();
   if (theFont)
   {
      theFont->getBoundingBox(theBoundingRect);
   }
   theBoundingRect += (theCenterPosition - theBoundingRect.midPoint());
   theCenterPosition = theBoundingRect.midPoint();                  
}

void ossimAnnotationFontObject::setGeometryInformation(const ossimFontInformation& info)
{
   thePixelSize       = info.thePointSize;
   theRotation        = info.theRotation;
   theHorizontalScale = info.theScale.x;
   theVerticalScale   = info.theScale.y;
   theHorizontalShear = info.theShear.x;
   theVerticalShear   = info.theShear.y;
   
   setFontInfo();
   if (theFont)
   {
      theFont->getBoundingBox(theBoundingRect);
   }
   theBoundingRect += (theCenterPosition - theBoundingRect.midPoint());
   theCenterPosition = theBoundingRect.midPoint();
}

void ossimAnnotationFontObject::applyScale(double x, double y)
{
   theCenterPosition.x = ossim::round<int>(theCenterPosition.x *x);
   theCenterPosition.y = ossim::round<int>(theCenterPosition.x *y);
   theHorizontalScale *= x;
   theVerticalScale   *= y;
   
   setFontInfo();
   if (theFont)
   {
      theFont->getBoundingBox(theBoundingRect);
   }
   theBoundingRect += (theCenterPosition - theBoundingRect.midPoint());
   theCenterPosition = theBoundingRect.midPoint();
}

ossimObject* ossimAnnotationFontObject::dup()const
{
   return new ossimAnnotationFontObject(*this);
}
