/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <OGRE/OgreSceneManager.h>
#include <OGRE/OgreSceneNode.h>

#include <ros/time.h>

#include <laser_geometry/laser_geometry.h>

#include "point_cloud.h"
#include "point_cloud_common.h"
#include <rviz/display_context.h>
#include <rviz/frame_manager.h>
#include <rviz/properties/int_property.h>
#include <rviz/validate_floats.h>

#include "emoji_laser_scan_display.h"

namespace emojicloud_plugin {
EmojiLaserScanDisplay::EmojiLaserScanDisplay()
    : point_cloud_common_(new PointCloudCommon(this)),
      projector_(new laser_geometry::LaserProjection()) {}

EmojiLaserScanDisplay::~EmojiLaserScanDisplay() {
  delete point_cloud_common_;
  delete projector_;
}

void EmojiLaserScanDisplay::onInitialize() {
  // Use the threaded queue for processing of incoming messages
  update_nh_.setCallbackQueue(context_->getThreadedQueue());

  MFDClass::onInitialize();
  point_cloud_common_->initialize(context_, scene_node_);

  static bool resource_locations_added = false;
  if (!resource_locations_added) {
    const std::string my_path =
        ros::package::getPath(ROS_PACKAGE_NAME) + "/shaders";
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        my_path, "FileSystem", ROS_PACKAGE_NAME);
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
    resource_locations_added = true;
  }
}

void EmojiLaserScanDisplay::processMessage(
    const sensor_msgs::LaserScanConstPtr &scan) {
  sensor_msgs::PointCloud2Ptr cloud(new sensor_msgs::PointCloud2);

  // Compute tolerance necessary for this scan
  ros::Duration tolerance(scan->time_increment * scan->ranges.size());
  if (tolerance > filter_tolerance_) {
    filter_tolerance_ = tolerance;
    tf_filter_->setTolerance(filter_tolerance_);
  }

  try {
    auto tf = context_->getTF2BufferPtr();

    projector_->transformLaserScanToPointCloud(
        fixed_frame_.toStdString(), *scan, *cloud, *tf, -1.0,
        laser_geometry::channel_option::Intensity);
  } catch (tf2::TransformException &e) {
    ROS_DEBUG("LaserScan [%s]: failed to transform scan: %s.  This message "
              "should not repeat (tolerance "
              "should now be set on our tf2::MessageFilter).",
              qPrintable(getName()), e.what());
    return;
  }

  point_cloud_common_->addMessage(cloud);
}

void EmojiLaserScanDisplay::update(float wall_dt, float ros_dt) {
  point_cloud_common_->update(wall_dt, ros_dt);
}

void EmojiLaserScanDisplay::reset() {
  MFDClass::reset();
  point_cloud_common_->reset();
}

} // namespace emojicloud_plugin

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(emojicloud_plugin::EmojiLaserScanDisplay, rviz::Display)
