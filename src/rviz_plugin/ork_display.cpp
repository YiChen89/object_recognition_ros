/*
 * Copyright (c) 2012, Willow Garage, Inc.
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

#include <boost/foreach.hpp>

#include <OGRE/OgreSceneNode.h>
#include <OGRE/OgreSceneManager.h>

#include <tf/transform_listener.h>

#include <rviz/visualization_manager.h>
#include <rviz/properties/color_property.h>
#include <rviz/properties/float_property.h>
#include <rviz/properties/int_property.h>
#include <rviz/frame_manager.h>

#include "ork_visual.h"

#include "ork_display.h"

namespace object_recognition_ros
{

// BEGIN_TUTORIAL
// The constructor must have no arguments, so we can't give the
// constructor the parameters it needs to fully initialize.
  OrkObjectDisplay::OrkObjectDisplay()
  {
    color_property_ = new rviz::ColorProperty("Color", QColor(204, 51, 204), "Color to draw the acceleration arrows.",
                                              this, SLOT(updateColorAndAlpha()));

    alpha_property_ = new rviz::FloatProperty("Alpha", 1.0, "0 is fully transparent, 1.0 is fully opaque.", this,
                                              SLOT(updateColorAndAlpha()));

    history_length_property_ = new rviz::IntProperty("History Length", 1, "Number of prior measurements to display.",
                                                     this, SLOT(updateHistoryLength()));
    history_length_property_->setMin(1);
    history_length_property_->setMax(100000);
  }

// After the top-level rviz::Display::initialize() does its own setup,
// it calls the subclass's onInitialize() function.  This is where we
// instantiate all the workings of the class.  We make sure to also
// call our immediate super-class's onInitialize() function, since it
// does important stuff setting up the message filter.
//
//  Note that "MFDClass" is a typedef of
// ``MessageFilterDisplay<message type>``, to save typing that long
// templated class name every time you need to refer to the
// superclass.
  void
  OrkObjectDisplay::onInitialize()
  {
    MFDClass::onInitialize();
    updateHistoryLength();
  }

  OrkObjectDisplay::~OrkObjectDisplay()
  {
  }

// Clear the visuals by deleting their objects.
  void
  OrkObjectDisplay::reset()
  {
    MFDClass::reset();
    visuals_.clear();
    visuals_.resize(history_length_property_->getInt());
  }

// Set the current color and alpha values for each visual.
  void
  OrkObjectDisplay::updateColorAndAlpha()
  {
    float alpha = alpha_property_->getFloat();
    Ogre::ColourValue color = color_property_->getOgreColor();

    for (size_t i = 0; i < visuals_.size(); i++)
    {
      if (visuals_[i])
      {
        visuals_[i]->setColor(color.r, color.g, color.b, alpha);
      }
    }
  }

// Set the number of past visuals to show.
  void
  OrkObjectDisplay::updateHistoryLength()
  {
    int new_length = history_length_property_->getInt();

    // Create a new array of visual pointers, all NULL.
    std::vector<boost::shared_ptr<OrkObjectVisual> > new_visuals(new_length);

    // Copy the contents from the old array to the new.
    // (Number to copy is the minimum of the 2 vector lengths).
    size_t copy_len = (new_visuals.size() > visuals_.size()) ? visuals_.size() : new_visuals.size();
    for (size_t i = 0; i < copy_len; i++)
    {
      int new_index = (messages_received_ - i) % new_visuals.size();
      int old_index = (messages_received_ - i) % visuals_.size();
      new_visuals[new_index] = visuals_[old_index];
    }

    // We don't need to create any new visuals here, they are created as
    // needed when messages are received.

    // Put the new vector into the member variable version and let the
    // old one go out of scope.
    visuals_.swap(new_visuals);
  }

// This is our callback to handle an incoming message.
  void
  OrkObjectDisplay::processMessage(const object_recognition_msgs::RecognizedObjectArrayConstPtr& msg)
  {
    // Here we call the rviz::FrameManager to get the transform from the
    // fixed frame to the frame in the header of this message. If
    // it fails, we can't do anything else so we return.
    Ogre::Quaternion orientation;
    Ogre::Vector3 position;
    if (!context_->getFrameManager()->getTransform(msg->header.frame_id, msg->header.stamp, position, orientation))
    {
      ROS_DEBUG(
          "Error transforming from frame '%s' to frame '%s'", msg->header.frame_id.c_str(), qPrintable( fixed_frame_ ));
      return;
    }

    visuals_.clear();
    BOOST_FOREACH(const object_recognition_msgs::RecognizedObject& object, msg->objects) {
      boost::shared_ptr<OrkObjectVisual> visual = boost::shared_ptr<OrkObjectVisual>(new OrkObjectVisual(context_->getSceneManager(), scene_node_));
      visuals_.push_back(visual);

      // Now set or update the contents of the chosen visual.
      visual->setMessage(object);
      visual->setFramePosition(position);
      visual->setFrameOrientation(orientation);

      float alpha = alpha_property_->getFloat();
      Ogre::ColourValue color = color_property_->getOgreColor();
      visual->setColor(color.r, color.g, color.b, alpha);
    }
  }
} // end namespace object_recognition_ros

// Tell pluginlib about this class.  It is important to do this in
// global scope, outside our package's namespace.
#include <pluginlib/class_list_macros.h>
PLUGINLIB_EXPORT_CLASS( object_recognition_ros::OrkObjectDisplay, rviz::Display)
// END_TUTORIAL
