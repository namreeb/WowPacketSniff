#pragma once

class CMovement_C
{
    private:
        unsigned int Unk0;
    public:
        CMovement_C *Next;
    private:
        unsigned int Unk1[2];
    public:
        float ClientPosition[3];
        float ClientOrientation;
        float ClientPitch;

        float MaybeGroundNormal[3];
        unsigned int *Descriptors;
    private:
        unsigned int Unk34;
    public:
        unsigned __int64 TransportGuid;
        unsigned int Flags;
        unsigned int Flags2;
        float LastServerPosition[3];
        float LastServerOrientation;
        float LastServerPitch;
        unsigned int TimeSinceLastServerMove;
        float JumpSinAngle1;
        float JumpCosAngle1;
        float JumpUnkAlwaysZero;
        float Orientation2Cos;
        float Orientation2Sin;
        float Pitch2Cos;
        float Pitch2Sin;
        unsigned int FallTime;
        float FallSpeed;
        float SplineUnk1;
        float JumpXYSpeed;
        float WalkSpeed;
        float RunSpeed;
        float RunBackSpeed;
        float SwimSpeed;
        float SwimBackSpeed;
        float FlightSpeed;
        float FlightBackSpeed;
        float TurnSpeed;
        float JumpVelocity;
    private:
        void *Spline;
    public:
        unsigned int LocalTimeToApplyMovement;
        unsigned int PreviousMoveServerTime;
    private:
        float UnknownFloat4;
        float UnknownFloat5;
        unsigned int UnkC4;
        float m_collisionBoxHalfDepth;
        float m_collisionBoxHeight;
    public:
        float StepUpHeight;
    private:
        unsigned int SomeArrayOfTimeHistory[32];
        unsigned int SomeArrayNextIndex;
        int MaxRecentHistoryValue;
        unsigned int Unk15C;
        float UnknownFloat8;
        float Unk164;
        float UnknownFloat9;
        unsigned int Unk16C[2];
        unsigned int MovementEventQueue[3];
    public:
        void *Owner;

        void __thiscall ExecuteMovement(unsigned int timeNow, unsigned int lastUpdate);
};
