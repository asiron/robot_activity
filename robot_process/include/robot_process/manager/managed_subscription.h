#ifndef MANAGED_SUBSCRIPTION_H
#define MANAGED_SUBSCRIPTION_H

#include <ros/ros.h>

#include <atomic>

//#include <robot_process/manager/managed_subscription_impl.h>

namespace robot_process {

class ManagedSubscription
{
public:

  ManagedSubscription();

  ManagedSubscription(const ManagedSubscription& rhs);
  ~ManagedSubscription() { };

  void subscribe(const ros::NodeHandlePtr& node_handle);
  void unsubscribe();

  void pause();
  void resume();

private:
  class Impl
  {
  public:
    Impl();
    ~Impl();

    void subscribe(const ros::NodeHandlePtr& node_handle);
    void unsubscribe();

    void pause();
    void resume();

    std::atomic<bool> subscribed_;
    std::atomic<bool> paused_;

    ros::Subscriber subscriber_;

    template <class Message>
    using Callback = boost::function<void(Message)>;

    template<class Message>
    Callback<Message> wrapCallback(const Callback<Message>& callback) const
    {
      return [this, &callback](Message message) {
        ROS_DEBUG("wrapped callback executed!");
        if (!paused_)
          callback(message);
        else
          ROS_DEBUG("callback is paused!");
      };
    }

    typedef std::function<ros::Subscriber(const ros::NodeHandlePtr&)> LazySubscribe;
    LazySubscribe lazy_subscribe_;

    template<class Message>
    LazySubscribe makeLazySubscribe(
      const std::string& topic, uint32_t queue_size,
      const Callback<Message>& callback,
      const ros::VoidConstPtr& tracked_object = ros::VoidConstPtr(),
      const ros::TransportHints& transport_hints = ros::TransportHints())
    {
      ROS_DEBUG("makeLazySubscribe Callback<Message>& callback form exec");
      return [=](const ros::NodeHandlePtr& nh) -> ros::Subscriber {
        ROS_DEBUG("Subscribing...");
        return nh->subscribe<Message>(
          topic,
          queue_size,
          static_cast<Callback<Message>>(wrapCallback(callback)),
          tracked_object,
          transport_hints);
      };
    }

    template<class M, class T>
    LazySubscribe makeLazySubscribe(
      const std::string& topic, uint32_t queue_size, void(T::*fp)(M), T* obj,
      const ros::TransportHints& transport_hints = ros::TransportHints())
    {
      ROS_DEBUG("makeLazySubscribe void(T::*fp)(M), T* obj, form exec");
      Callback<M> callback = boost::bind(fp, obj, _1);
      return makeLazySubscribe(topic, queue_size, callback, ros::VoidConstPtr(), transport_hints);
    }

  };

  template<typename... Args>
  ManagedSubscription(Args&& ...args)
    : impl_(std::make_shared<Impl>())
  {
    impl_->lazy_subscribe_ = impl_->makeLazySubscribe(std::forward<Args>(args)...);
  }

  typedef std::shared_ptr<Impl> ImplPtr;
  ImplPtr impl_;

  friend class SubscriptionManager;

};

} // namespace robot_process

#endif
