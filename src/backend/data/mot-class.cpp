#
/*
 *    Copyright (C) 2015 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	MOT handling is a crime, here we have a single class responsible
 *	for handling a single MOT message with a given transportId
 */
#include	"mot-class.h"
#include	"radio.h"

	   motClass::motClass (RadioInterface *mr,
	                       QString	picturePath,
	                       int16_t	transportId,
	                       uint8_t	*segment,
	                       int32_t	segmentSize,
	                       bool		lastFlag) {
int32_t pointer = 7;

	connect (this, SIGNAL (the_picture (QByteArray, int, QString)),
	         mr,   SLOT   (showMOT     (QByteArray, int, QString)));
	this	-> transportId		= transportId;
	this	-> numofSegments	= -1;
	this	-> segmentSize		= -1;

	headerSize     =
             ((segment [3] & 0x0F) << 9) | (segment [4]) | (segment [5] >> 7);
	bodySize       =
              (segment [0] << 20) | (segment [1] << 12) |
                            (segment [2] << 4 ) | ((segment [3] & 0xF0) >> 4);
	contentType     = ((segment [5] >> 1) & 0x3F);
	contentsubType	= ((segment [5] & 0x01) << 8) | segment [6];
//
//	we are actually only interested in the name, if any
        while (pointer < headerSize) {
           uint8_t PLI	= (segment [pointer] & 0300) >> 6;
           uint8_t paramId = (segment [pointer] & 077);
           uint16_t     length;
           switch (PLI) {
              case 00:
                 pointer += 1;
                 break;

              case 01:
                 pointer += 2;
                 break;

	      case 02:
                 pointer += 5;
                 break;

              case 03:
                 if ((segment [pointer + 1] & 0200) != 0) {
                    length = (segment [pointer + 1] & 0177) << 8 |
                              segment [pointer + 2];
                    pointer += 3;
                 }
                 else {
                    length = segment [pointer + 1] & 0177;
                    pointer += 2;
                 }
	         if (paramId == 12) {
                    int16_t i;
                    for (i = 0; i < length - 1; i ++)
                       name. append (segment [pointer + i + 1]);
                 }
                 pointer += length;
           }
	}

	for (int i = 0; i < 128; i ++)
	   marked [i] = false;
}

	motClass::~motClass	(void) {
}

uint16_t	motClass::get_transportId (void) {
	return transportId;
}

//      type 4 is a segment.
//	The pad software will only call this whenever it has
//	established that the current slide has th right transportId
//
void	motClass::addBodySegment (uint8_t	*bodySegment,
	                            int16_t	segmentNumber,
	                            int32_t	segmentSize,
	                            bool	lastFlag) {
int32_t i;

        if (marked [segmentNumber])   // we already have the segment
           return;

//      Note that the last segment may have a different size
        if (!lastFlag && (this -> segmentSize == -1))
           this -> segmentSize = segmentSize;
//
        segments [segmentNumber]. resize (segmentSize);
        for (i = 0; i < segmentSize; i ++)
           segments [segmentNumber][i] = bodySegment [i];
        marked [segmentNumber] = true;
//
        if (lastFlag)
           numofSegments = segmentNumber + 1;

	if (numofSegments == -1)
	   return;
//
//	once we know how many segments there are/should be,
//	we check for completeness
	for (i = 0; i < numofSegments; i ++) 
           if (!(marked [i]))
	      return;

//	if we are complete, handle the content
	if (contentType != 2)
	   return;

	QByteArray result;
	for (i = 0; i < numofSegments; i ++)
	   result. append (segments [i]);

	QString realName = picturePath;
	if (name == QString (""))
           realName. append (QString ("no name"));
        else
           realName. append (name);
	checkDir (realName);
	the_picture (result, contentsubType, realName);
}

void	motClass::checkDir (QString &s) {
int16_t	ind	= s. lastIndexOf (QChar ('/'));
int16_t	i;
QString	dir;

	if (ind == -1)		// no slash, no directory
	   return;

	for (i = 0; i < ind; i ++)
	   dir. append (s [i]);

	if (QDir (dir). exists ())
	   return;
	QDir (). mkpath (dir);
}

