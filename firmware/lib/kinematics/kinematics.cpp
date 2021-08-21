// Copyright (c) 2021 Juan Miguel Jimeno
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Arduino.h"
#include "kinematics.h"

Kinematics::Kinematics(base robot_base, int motor_max_rpm, float max_rpm_ratio,
                       float wheel_diameter, float wheels_y_distance):
    base_platform(robot_base),
    max_rpm_(motor_max_rpm * max_rpm_ratio),
    wheels_y_distance_(wheels_y_distance),
    wheel_circumference_(PI * wheel_diameter),
    total_wheels_(getTotalWheels(robot_base))
{    
}

Kinematics::rpm Kinematics::calculateRPM(float linear_x, float linear_y, float angular_z)
{

    float tangential_vel = angular_z * (wheels_y_distance_ / 2.0);

    //convert m/s to m/min
    float linear_vel_x_mins = linear_x * 60.0;
    float linear_vel_y_mins = linear_y * 60.0;
    //convert rad/s to rad/min
    float tangential_vel_mins = tangential_vel * 60.0;

    float x_rpm = linear_vel_x_mins / wheel_circumference_;
    float y_rpm = linear_vel_y_mins / wheel_circumference_;
    float tan_rpm = tangential_vel_mins / wheel_circumference_;

    float xy_sum = x_rpm + y_rpm;
    float xtan_sum = x_rpm + tan_rpm;

    //calculate the scale value how much each target velocity
    //must be scaled down in such cases where the total required RPM
    //is more than the motor's max RPM
    //this is to ensure that the required motion is achieved just with slower speed
    if(((xy_sum) > max_rpm_ ) || ((xy_sum < -max_rpm_)))
    {
        float vel_scaler = (float)max_rpm_ / xy_sum;
        x_rpm *= vel_scaler;
        y_rpm *= vel_scaler;
    }
    
    if(((xtan_sum) > max_rpm_ ) || ((xtan_sum < -max_rpm_)))
    {
        float vel_scaler = (float)max_rpm_ / xtan_sum;
        x_rpm *= vel_scaler;
        tan_rpm *= vel_scaler;
    }

    Kinematics::rpm rpm;

    //calculate for the target motor RPM and direction
    //front-left motor
    rpm.motor1 = x_rpm - y_rpm - tan_rpm;
    rpm.motor1 = constrain(rpm.motor1, -max_rpm_, max_rpm_);

    //front-right motor
    rpm.motor2 = x_rpm + y_rpm + tan_rpm;
    rpm.motor2 = constrain(rpm.motor2, -max_rpm_, max_rpm_);

    //rear-left motor
    rpm.motor3 = x_rpm + y_rpm - tan_rpm;
    rpm.motor3 = constrain(rpm.motor3, -max_rpm_, max_rpm_);

    //rear-right motor
    rpm.motor4 = x_rpm - y_rpm + tan_rpm;
    rpm.motor4 = constrain(rpm.motor4, -max_rpm_, max_rpm_);

    return rpm;
}

Kinematics::rpm Kinematics::getRPM(float linear_x, float linear_y, float angular_z)
{
    if(base_platform == DIFFERENTIAL_DRIVE || base_platform == SKID_STEER)
    {
        linear_y = 0;
    }

    return calculateRPM(linear_x, linear_y, angular_z);;
}

Kinematics::velocities Kinematics::getVelocities(int rpm1, int rpm2, int rpm3, int rpm4)
{
    Kinematics::velocities vel;
    float average_rps_x;
    float average_rps_y;
    float average_rps_a;

    //convert average revolutions per minute to revolutions per second
    average_rps_x = ((float)(rpm1 + rpm2 + rpm3 + rpm4) / total_wheels_) / 60.0; // RPM
    vel.linear_x = average_rps_x * wheel_circumference_; // m/s

    //convert average revolutions per minute in y axis to revolutions per second
    average_rps_y = ((float)(-rpm1 + rpm2 + rpm3 - rpm4) / total_wheels_) / 60.0; // RPM
    if(base_platform == MECANUM)
        vel.linear_y = average_rps_y * wheel_circumference_; // m/s
    else
        vel.linear_y = 0;

    //convert average revolutions per minute to revolutions per second
    average_rps_a = ((float)(-rpm1 + rpm2 - rpm3 + rpm4) / total_wheels_) / 60.0;
    vel.angular_z =  (average_rps_a * wheel_circumference_) / (wheels_y_distance_ / 2); //  rad/s

    return vel;
}

int Kinematics::getTotalWheels(base robot_base)
{
    switch(robot_base)
    {
        case DIFFERENTIAL_DRIVE:    return 2;
        case SKID_STEER:            return 4;
        case MECANUM:               return 4;
        default:                    return 2;
    }
}