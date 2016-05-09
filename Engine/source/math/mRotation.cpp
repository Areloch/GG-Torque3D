//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------
#include "math/mRotation.h"

#ifdef TORQUE_TESTS_ENABLED
#include "testing/unitTesting.h"
#endif

//====================================================================
//Eulers setup
//====================================================================
RotationF::RotationF(EulerF _euler, UnitFormat format)
{
   set(_euler.x, _euler.y, _euler.z, format);
}

RotationF::RotationF(F32 _x, F32 _y, F32 _z, UnitFormat format)
{
   set(_x, _y, _z, format);
}

void RotationF::set(EulerF _euler, UnitFormat format)
{
   set(_euler.x, _euler.y, _euler.z, format);
}

void RotationF::set(F32 _x, F32 _y, F32 _z, UnitFormat format)
{
   EulerF tempEul;
   if (format == Degrees)
   {
      tempEul.set(mDegToRad(_x), mDegToRad(_y), mDegToRad(_z));
   }
   else
   {
      tempEul.set(_x, _y, _z);
   }

   mRotation.set(tempEul);
}

//====================================================================
//AxisAngle setup
//====================================================================
RotationF::RotationF(AngAxisF _aa, UnitFormat format)
{
   set(_aa, format);
}

void RotationF::set(AngAxisF _aa, UnitFormat format)
{
   if (format == Degrees)
   {
      _aa.angle = mDegToRad(_aa.angle);
   }

   mRotation.set(_aa);
}

//====================================================================
//QuatF setup
//====================================================================
RotationF::RotationF(QuatF _quat)
{
   set(_quat);
}

void RotationF::set(QuatF _quat)
{
   mRotation = _quat;
}

//====================================================================
//MatrixF setup
//====================================================================
RotationF::RotationF(MatrixF _mat)
{
   set(_mat);
}

void RotationF::set(MatrixF _mat)
{
   mRotation.set(_mat);
}

//
inline F32 RotationF::len() const
{
   return asEulerF().len();
}

inline void RotationF::interpolate(const RotationF& _from, const RotationF& _to, F32 _factor)
{
   mRotation.interpolate(_from.asQuatF(), _to.asQuatF(), _factor);
}

void RotationF::lookAt(const Point3F& origin, const Point3F& target, const Point3F& up)
{
   VectorF forwardVector = target - origin;
   forwardVector.normalize();

   //Torque uses +Y as forward
   VectorF forward(0, 1, 0);

   float dot = mDot(up, forwardVector);

   if (mIsZero(mAbs(dot - -1.0f)))
   {
      mRotation.set(up);
   }
   else if (mIsZero(mAbs(dot - 1.0f)))
   {
      mRotation = QuatF::Identity;
   }

   float rotAngle = mAcos(dot);
   VectorF rotAxis = mCross(up, forwardVector);
   rotAxis.normalize();

   mRotation.set(AngAxisF(rotAxis, rotAngle));
}

void RotationF::setPitchAngle(const F32& _pitch, UnitFormat format)
{
   F32 pitch = format == Radians ? _pitch : mDegToRad(_pitch);
   QuatF pitchRot = AngAxisF(Point3F(1, 0, 0), pitch);

   QuatF setRot;

   setRot.mul(pitchRot, mRotation);
   //setRot.normalize();

   mRotation = setRot;
}

void RotationF::setRollAngle(const F32& _roll, UnitFormat format)
{
   F32 roll = format == Radians ? _roll : mDegToRad(_roll);
   QuatF rollRot = AngAxisF(Point3F(0, 1, 0), roll);

   QuatF setRot;

   setRot.mul(rollRot, mRotation);
   //setRot.normalize();

   mRotation = setRot;
}

void RotationF::setYawAngle(const F32& _yaw, UnitFormat format)
{
   F32 yaw = format == Radians ? _yaw : mDegToRad(_yaw);
   QuatF yawRot = AngAxisF(Point3F(0, 0, 1), yaw);

   QuatF setRot;

   setRot.mul(yawRot, mRotation);
   //setRot.normalize();

   mRotation = setRot;
}

F32 RotationF::getPitchAngle(UnitFormat format)
{
   F32 yaw = atan2(2.0*(mRotation.y*mRotation.z + mRotation.w*mRotation.x),
      mRotation.w*mRotation.w - mRotation.x*mRotation.x - mRotation.y*mRotation.y + mRotation.z*mRotation.z);

   return format == Radians ? yaw : mRadToDeg(yaw);
}

F32 RotationF::getRollAngle(UnitFormat format)
{
   F32 pitch = asin(-2.0*(mRotation.x*mRotation.z - mRotation.w*mRotation.y));

   return format == Radians ? pitch : mRadToDeg(pitch);
}

F32 RotationF::getYawAngle(UnitFormat format)
{
   F32 roll = atan2(2.0*(mRotation.x*mRotation.y + mRotation.w*mRotation.z),
      mRotation.w*mRotation.w + mRotation.x*mRotation.x - mRotation.y*mRotation.y - mRotation.z*mRotation.z);

   return format == Radians ? roll : mRadToDeg(roll);
}

void RotationF::clampPitch(const F32& _min, const F32& _max, UnitFormat format)
{
   F32 min = format == Radians ? _min : mDegToRad(_min);
   F32 max = format == Radians ? _max : mDegToRad(_max);
   F32 pitch = getPitchAngle();

   pitch = mClampF(pitch, min, max);

   setPitchAngle(pitch);
}

void RotationF::clampRoll(const F32& _min, const F32& _max, UnitFormat format)
{
   F32 min = format == Radians ? _min : mDegToRad(_min);
   F32 max = format == Radians ? _max : mDegToRad(_max);
   F32 roll = getRollAngle();

   roll = mClampF(roll, min, max);

   setRollAngle(roll);
}

void RotationF::clampYaw(const F32& _min, const F32& _max, UnitFormat format)
{
   F32 min = format == Radians ? _min : mDegToRad(_min);
   F32 max = format == Radians ? _max : mDegToRad(_max);
   F32 yaw = getYawAngle();

   yaw = mClampF(yaw, min, max);

   setYawAngle(yaw);
}

//========================================================
EulerF RotationF::asEulerF(UnitFormat format) const
{
   double sqw;
   double sqx;
   double sqy;
   double sqz;

   double rotxrad;
   double rotyrad;
   double rotzrad;

   sqw = mRotation.w * mRotation.w;
   sqx = mRotation.x * mRotation.x;
   sqy = mRotation.y * mRotation.y;
   sqz = mRotation.z * mRotation.z;

   EulerF returnEuler;

   returnEuler.x = (double)atan2l(2.0 * (mRotation.y * mRotation.z + mRotation.x * mRotation.w), (-sqx - sqy + sqz + sqw));
   returnEuler.y = (double)asinl(-2.0 * (mRotation.x * mRotation.z - mRotation.y * mRotation.w));
   returnEuler.z = (double)atan2l(2.0 * (mRotation.x * mRotation.y + mRotation.z * mRotation.w), (sqx - sqy - sqz + sqw));

   if (format == Degrees)
   {
      returnEuler.x = mRadToDeg(returnEuler.x);
      returnEuler.y = mRadToDeg(returnEuler.y);
      returnEuler.z = mRadToDeg(returnEuler.z);
   }

   return returnEuler;
}

AngAxisF RotationF::asAxisAngle(UnitFormat format) const
{
   AngAxisF returnAA;
   returnAA.set(mRotation);

   if (format == Radians)
   {
      returnAA.angle = mDegToRad(returnAA.angle);
   }

   return returnAA;
}

MatrixF RotationF::asMatrixF() const
{
   MatrixF returnMat;
   mRotation.setMatrix(&returnMat);

   return returnMat;
}

QuatF RotationF::asQuatF() const
{
   return mRotation;
}

void RotationF::normalize()
{
   mRotation.normalize();
}

EulerF QuatToEuler(const QuatF *quat)
{
   double sqw;
   double sqx;
   double sqy;
   double sqz;

   double rotxrad;
   double rotyrad;
   double rotzrad;

   sqw = quat->w * quat->w;
   sqx = quat->x * quat->x;
   sqy = quat->y * quat->y;
   sqz = quat->z * quat->z;

   EulerF returnEul;

   returnEul.x = (double)atan2l(2.0 * (quat->y * quat->z + quat->x * quat->w), (-sqx - sqy + sqz + sqw));
   returnEul.y = (double)asinl(-2.0 * (quat->x * quat->z - quat->y * quat->w));
   returnEul.z = (double)atan2l(2.0 * (quat->x * quat->y + quat->z * quat->w), (sqx - sqy - sqz + sqw));

   return returnEul;
}

//Testing
#ifdef TORQUE_TESTS_ENABLED
TEST(Maths, RotationF_Calculations)
{
   //TODO: implement unit test
};
#endif