#pragma once

namespace globals {

template <typename T>
class GlobalObject
{
public:
    static void set(T* _obj) { m_obj = _obj; }
	static T & getRef() { return *m_obj; }
	static T * getPtr() { return m_obj; }

private:
	static T* m_obj;
};

template <typename T>
T* GlobalObject<T>::m_obj;

template <typename T>
inline T & getRef()
{
	return GlobalObject<T>::getRef();
}

template <typename T>
inline T * getPtr()
{
	return GlobalObject<T>::getPtr();

}

void init();
void deinit();

}