#include <iostream>
#include <string>
#include <functional>
#include <map>
#include <vector>
#include <typeindex>
#include <tuple>

class CameraData {
    public: 
        CameraData() : data("Class::CameraData") {}
        std::string data;
};

class MotorData {
    public: 
        MotorData() : data("Class::MotorData") {}
        std::string data;
};

class ReactorControl {
    public:
        template <typename T>
        T* get() {
            return static_cast<T*>(m_data[typeid(T)]);
        }

        template <typename T>
        void emit(T* data) {
            m_data[typeid(T)] = data;
        }
    private:
        std::map<std::type_index, void*> m_data;
};

class Reactor {
    public:
        Reactor(ReactorControl& reactorControl) :
            reactorControl(reactorControl){

        }

        template <typename TTrigger>
        void notify() {
            auto& callbacks = getCallbackList<TTrigger>();
            for(auto callback = std::begin(callbacks); callback != std::end(callbacks); ++callback) {
                (*callback)();
            }
        }
    protected:
        template <typename... TTriggers>
        class Trigger {};

        template <typename... TWith>
        class With {};

        template <typename TTrigger, typename TFunc>
        void on(TFunc callback) {
            OnImpl<TTrigger, With<>, TFunc>(this)(callback);
        }

        template <typename TTrigger, typename TWith, typename TFunc>
        void on(TFunc callback) {
            OnImpl<TTrigger, TWith, TFunc>(this)(callback);
        }
    private:
        // Base case -- Should never be instantiated and if it is it's a compile error
        template < typename, typename, typename > 
        struct OnImpl {};

        // Trigger & With case, requires Trigger<...> and With<...>
        template <typename... TTriggers, typename... TWiths, typename TFunc>
        struct OnImpl<Trigger<TTriggers...>, With<TWiths...>, TFunc> {
            Reactor* context;

            OnImpl(Reactor* context) {
                this->context = context;
            }

            void operator()(TFunc callback) {
                std::cerr << "Superspecialized Reactor::OnImpl::operator() [size: " << sizeof...(TTriggers) << ", " << sizeof...(TWiths) << "]" << std::endl;
                context->bindTriggers<TTriggers...>(
                    context->buildCallback<TFunc, TTriggers..., TWiths...>(callback) 
                );
            }
        };

        template <typename TFunc, typename... TTriggersAndWiths>
        std::function<void ()> buildCallback(TFunc callback) {
            std::function<void ()> wrappedCallback([this, callback]() {
                callback((*reactorControl.get<TTriggersAndWiths>())...);
                //callback();
            });

            return wrappedCallback;
        }

        template <typename TTriggerFirst>
        void bindTriggers(std::function<void ()> callback) {
            bindTriggersImpl<TTriggerFirst>(callback, reinterpret_cast<TTriggerFirst*>(0));
        }

        // Recursive case -- See: http://stackoverflow.com/a/5035973/203133
        template <typename TTriggerFirst, typename TTriggerSecond, typename... TTriggers>
        void bindTriggers(std::function<void ()> callback) {
            bindTriggers<TTriggerFirst>(callback);
            bindTriggers<TTriggerSecond, TTriggers...>(callback);
        }

        template <typename TTrigger>
        void bindTriggersImpl(std::function<void ()> callback, TTrigger*) {
            auto& callbacks = getCallbackList<TTrigger>();
            callbacks.push_back(callback);
        }

        template <typename TTrigger>
        std::vector<std::function<void ()>>& getCallbackList() {
            if(m_callbacks.find(typeid(TTrigger)) == m_callbacks.end()) {
                m_callbacks[typeid(TTrigger)] = std::vector<std::function<void ()>>();
            }

            return m_callbacks[typeid(TTrigger)];
        }

        std::map<std::type_index, std::vector<std::function<void ()>>> m_callbacks;
        ReactorControl& reactorControl;
};

void react(const CameraData& cameraData, const MotorData& motorData) {
    std::cout << "Reacing on cameraData, got: [" << cameraData.data << "], [" << motorData.data << "]" << std::endl;
}

class Vision : public Reactor {
    public:
        Vision(ReactorControl& control) : Reactor(control) {
            //on<Trigger<MotorData>>();
            on<Trigger<CameraData>, With<MotorData>>(&react);
            on<Trigger<CameraData, MotorData>>([](const CameraData& cameraData, const MotorData& motorData) {
                std::cout << "Double trigger!" << std::endl;
            });

            on<Trigger<CameraData>, With<MotorData>>(std::bind(&Vision::reactInner, this, std::placeholders::_1, std::placeholders::_2));
        }
    private:
        void reactInner(const CameraData& cameraData, const MotorData& motorData) {
            std::cout << "ReactInner on cameraData, got: [" << cameraData.data << "], [" << motorData.data << "]" << std::endl;
        }
};

int main(int argc, char** argv) {
    ReactorControl control; 
    control.emit(new CameraData());
    control.emit(new MotorData());
    //OnImpl< int, int >()();
    //OnImpl< std::tuple<int, float, int>, std::tuple<int, float, float> >()();
    //OnImpl< Trigger<int, float>, With<int> >()();
    Vision vision(control);
    vision.notify<CameraData>();
    vision.notify<MotorData>();
}
