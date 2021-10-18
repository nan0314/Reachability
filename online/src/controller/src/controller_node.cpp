#include <ros/ros.h>
#include <ros/package.h>
#include <iostream>
#include <cstdio>
#include <Eigen/Core>
#include "controller/P5Dubins.hpp"
#include "nav_msgs/Path.h"
#include "trajectories/Path.h"
#include "trajectories/Trajectory.h"
#include "visualization_msgs/Marker.h"
#include "visualization_msgs/MarkerArray.h"

static nav_msgs::Path path;
static bool path_recieved;
static bool arrived;

// void path_callback(const nav_msgs::Path& msg){

//   if (path.poses.size() != msg.poses.size()){
//     path = msg;
//     path_recieved = true;
//     arrived = false;
//   } else if (msg.poses.size() > 0){
//     for (int i = 0; i < msg.poses.size(); i++){
//       if (msg.poses[i].pose.position.x != path.poses[i].pose.position.x){
//         path = msg;
//         path_recieved = true;
//         arrived = false;
//         break;
//       }
//     }
//   }   

// }

int main(int argc, char **argv)
{
    using std::string;
    using std::vector;
    using visualization_msgs::Marker;
    using visualization_msgs::MarkerArray;
    using arma::vec;
    using namespace std;
    using namespace Eigen;

    // initialize node/node handles
    ros::init(argc, argv, "controller_node");
    ros::NodeHandle n;

    // publishers and subscribers
    ros::Publisher pose_pub = n.advertise<MarkerArray>("/pose", 10,true);
    // ros::Subscriber path_sub = n.subscribe("path", 10, path_callback);

    // ROS parameters
    vector<double> model_size;
    vector<double> B;
    string frame_id;

    n.getParam("model_size", model_size);
    n.getParam("B", B);
    n.getParam("frame_id",frame_id);


    // initialize messages
    MarkerArray pose_msg;
    Marker vehicle;
    Marker TEB;

    vehicle.header.frame_id = frame_id;
    vehicle.id = 0;
    vehicle.type = 1;
    vehicle.pose.position.x = 3;
    vehicle.pose.position.y = 3;
    vehicle.scale.x = model_size[0];
    vehicle.scale.y = model_size[1];
    vehicle.scale.z = 0.05;
    vehicle.color.r = 0.0;
    vehicle.color.g = 0.0;
    vehicle.color.b = 1.0;
    vehicle.color.a = 1.0;

    TEB.header.frame_id = frame_id;
    TEB.id = 1;
    TEB.type = 1;
    TEB.pose = vehicle.pose;
    TEB.scale.x = model_size[0] + B[0];
    TEB.scale.y = model_size[1] + B[1];
    TEB.scale.z = 0.025;
    TEB.color.r = 1.0;
    TEB.color.g = 0.0;
    TEB.color.b = 0.0;
    TEB.color.a = 0.5;
    
    // set publishing frequency
    ros::Rate loop_rate(10);

    double dt = 0.1;
    arrived = true;

    int count = 0;
    while (ros::ok())
    {
        ros::spinOnce();

        pose_msg.markers = {vehicle,TEB};
        pose_pub.publish(pose_msg);


        loop_rate.sleep();
        ++count;
  }


  return 0;
}