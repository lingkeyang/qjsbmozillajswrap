#include "mozjswrap.h"

// objMap
///////////////////////////////////////////////////////////////////////////////////////////////////

// objMap::OBJMAP objMap::mMap;
// OBJID objMap::lastID = 1; // starts from 1, 0 means nothing
// 
// void objMap::trace(JSTracer *trc)
// {
//     char sz[16] = {'o','m',0};
//     OBJMAPIT it = mMap.begin();
//     for (; it != mMap.end(); it++)
//     {
//         itoa(it->first, &sz[2], 10);
//         JS::Heap<JSObject*>* p = &it->second.obj;
//         if (!objRoot::containsValue((JSObject*)(*p)))
//             JS_CallHeapObjectTracer(trc, p, sz);
//     }
// }
// 
// OBJID objMap::add(JS::HandleObject jsObj)
// {
//     OBJID id = lastID++;
//     mMap[id] = stHeapObj(jsObj);
//     return id;
// }
// 
// OBJID objMap::jsObj2ID(JS::HandleObject jsObj, bool autoAdd)
// {
// 	if (jsObj == 0)
// 	{
// 		return 0;
// 	}
//     OBJMAPIT it = mMap.begin();
//     for (; it != mMap.end(); it++)
//     {
//         if (it->second.obj == jsObj)
//         {
//             return it->first;
//         }
//     }
// 	if (autoAdd)
// 	{
// 		return add(jsObj);
// 	}
//     return 0;
// }
// 
// JSObject* objMap::id2JSObj(OBJID id)
// {
//     OBJMAPIT it = mMap.find(id);
//     if (it != mMap.end())
//     {
//         return it->second.obj;
//     }
//     return 0;
// }
// 
// bool objMap::remove(OBJID id)
// {
//     OBJMAPIT it = mMap.find(id);
//     if (it != mMap.end())
//     {
//         mMap.erase(it);
//         return true;
//     }
//     return false;
// }

// valueArr
///////////////////////////////////////////////////////////////////////////////////////////////////

//
// for
// 1) Array
// 2) Function Arguments
// 
int* valueArr::arr = 0;
int valueArr::Capacity = 0;
int valueArr::lastIndex = -1;
int* valueArr::makeSureArrHeapObj(int index)
{
    int size = index + 1;
    int& S = Capacity;
    if (arr == 0 || S < size) 
    {
        int oldS = S;
        if (S == 0)
            S = 8;
        while (S < size)
            S *= 2;

        // copy data
        int* _arr = new int[size];
        if (index > 0)
        {
            int N = min(oldS - 1, index);
            for (int i = 0; i < N; i++)
                _arr[i] = arr[i];
        }
        if (arr)
        {
            delete[] arr;
        }
        arr = _arr;
    }
    return arr;
}

void valueArr::add(int arrIndex, MAPID iMap)
{
    lastIndex = arrIndex;
    int* arr = makeSureArrHeapObj(arrIndex);
    arr[arrIndex] = iMap;
}

void valueArr::clear(bool bClear)
{
    if (bClear)
    {
        int& index = lastIndex;
        if (index >= 0)
        {
            Assert(index <= Capacity - 1);
            int N = min(index, Capacity - 1);
            for (int i = 0; i <= N; i++)
            {
                valueMap::removeByID(arr[i], false);
            }
        }
    }
    lastIndex = -1;
}

// objRoot
///////////////////////////////////////////////////////////////////////////////////////////////////

// std::map<OBJID, JSObject**> objRoot::mapObjectRoot;
// bool objRoot::add( OBJID id )
// {
//     if (mapObjectRoot.find(id) != mapObjectRoot.end())
//     {
//         return true;
//     }
//     JSObject* jsObj = objMap::id2JSObj(id);
//     if (jsObj == 0)
//         return false;
//     JSObject** pjsObj = new JSObject*;
//     *pjsObj = jsObj;
//     bool ret = JS_AddObjectRoot(g_cx, pjsObj);
//     mapObjectRoot[id] = pjsObj;
//     return ret;
// }
// 
// bool objRoot::remove( OBJID id )
// {
//     std::map<OBJID, JSObject**>::iterator it = mapObjectRoot.find(id);
//     if (it != mapObjectRoot.end())
//     {
//         JSObject** pjsObj = it->second;
//         JS_RemoveObjectRoot(g_cx, pjsObj);
//         delete pjsObj;
//         mapObjectRoot.erase(it);
//         return true;
//     }
//     return false;
// }
// 
// bool objRoot::containsValue(JSObject* obj)
// {
//     std::map<OBJID, JSObject**>::iterator it = mapObjectRoot.begin();
//     for (; it != mapObjectRoot.end(); it++)
//     {
//         if (*(it->second) == obj)
//             return true;
//     }
//     return false;
// 
// }

valueMap::VALUEMAP valueMap::mMap;
valueMap::VMAP valueMap::VMap;
int valueMap::index = 1;
bool valueMap::tracing = false;
std::list<int> valueMap::LstTempID;

void valueMap::trace(JSTracer *trc)
{
    Assert(!valueMap::tracing);
    valueMap::tracing = true;

	//VMap.clear();

	VMAP::iterator vit;
	uint64_t Old, New;

    //char sz[16] = {'v','m',0};
    VALUEMAPIT it = mMap.begin();
    for (; it != mMap.end(); it++)
    {
        //itoa(it->first, &sz[2], 10);
		//stHeapValue* p = it->second;
        stHeapValue* p = &it->second;
        
        if (p->bTrace || p->bTempTrace)
        {
			Old = p->heapValue.get().data.asBits;
			//
			// Set name to 0 is OK? it seems OK. I'm not sure.
			// 
            JS_CallHeapValueTracer(trc, &p->heapValue, 0 /* name */);
			New = p->heapValue.get().data.asBits;
			if (New != Old)
			{
				//
				// Will it get here?
				// Not sure
				//
				vit = VMap.find(Old);
				Assert(vit != VMap.end());
				VMap.erase(vit);
				VMap.insert(VMAP::value_type(New, it->first));
			}
        }
    }

// 	for (it = mMap.begin(); it != mMap.end(); it++)
// 	{
// 		VMap.insert(VMAP::value_type(it->second.heapValue.get().data.asBits, it->first));
// 	}

    valueMap::tracing = false;
}

MAPID valueMap::add(JS::HandleValue val, int mark)
{
    int ret = containsValue(val);
    if (ret != 0)
    {
        return ret;
    }

    //stHeapValue* p = new stHeapValue(val);
	stHeapValue p(val);
	p.mark = mark;

	int& J = valueMap::index;
    if (J == 0) J = 1;

	Assert(!valueMap::tracing);

	// 1)
	mMap.insert(VALUEMAP::value_type(J, p));
	//mMap[valueMap::index] = p;

	// 2)
	VMap.insert(VMAP::value_type(p.heapValue.get().data.asBits, J));
    
    return J++;
}

MAPID valueMap::addFunction(JS::HandleValue val)
{
    JS::RootedValue ns(g_cx);
    if (!JS_ConvertValue(g_cx, val, JSTYPE_FUNCTION, &ns))
        return 0;

    return add(ns, 8);
}

MAPID valueMap::containsValue(JS::Value v)
{
//     VALUEMAPIT it = mMap.begin();
//     for (; it != mMap.end(); it++)
//     {
//         if (v == it->second.heapValue.get())
//             return it->first;
//     
// 	}

	VMAP::iterator vit = VMap.find(v.data.asBits);
	if (vit != VMap.end())
	{
		VALUEMAPIT it = mMap.find(vit->second);
		Assert(it != mMap.end());
		return it->first;
	}


    return 0;
}

bool valueMap::setTrace(MAPID id, bool trace)
{
    VALUEMAPIT it = mMap.find(id);
    if (it != mMap.end())        
    {
		stHeapValue& hv = it->second;
        hv.bTrace = trace;
        return true;
    }
    Assert(false, "valueMap::setTrace fail");
    return false;
}

bool valueMap::setTempTrace(MAPID id, bool tempTrace)
{
    VALUEMAPIT it = mMap.find(id);
    if (it != mMap.end())
    {
        it->second.bTempTrace = tempTrace;
        return true;
    }
    Assert(false, "valueMap::setTempTrace fail");
    return false;
}

bool valueMap::setHasFinalizeOp(MAPID id, bool has)
{
    VALUEMAPIT it = mMap.find(id);
    if (it != mMap.end())
    {
        it->second.hasFinalizeOp = has;
        return true;
    }
    Assert(false, "valueMap::setHasFinalizeOp fail");
    return false;
}

bool valueMap::clear()
{
//     VALUEMAPIT it = mMap.begin();
//     for (; it != mMap.end(); it++)
//     {
//         delete it->second;
//     }
    mMap.clear();
	VMap.clear();
    return 0;
}

bool valueMap::removeByID( MAPID i, bool bForce )
{
    VALUEMAPIT it = mMap.find(i);
    if (it != mMap.end())
    {
        stHeapValue* p = &it->second;
        p->bTempTrace = false;
        Assert(!(p->bTrace && p->hasFinalizeOp), "trace and finalize are both true!!!");
        if (bForce || (!p->bTrace && !p->hasFinalizeOp))
        {
            Assert(!valueMap::tracing);

			// 1)
			VMAP::iterator vit = VMap.find(it->second.heapValue.get().data.asBits);
			Assert(vit != VMap.end());
			VMap.erase(vit);

            // 2)
            mMap.erase(it);

        }
        return true;
    }
    return false;
}
bool valueMap::getVal(MAPID id, JS::MutableHandleValue pVal)
{
    if (id == 0)
        return false;

    VALUEMAPIT it = mMap.find(id);
    if (it != mMap.end())
    {
        pVal.set(it->second.heapValue.get());
        return true;
    }
    return false;
}

MAPID valueMap::getID(const JS::Value& val, bool autoAdd)
{
//     VALUEMAPIT it = mMap.begin();
//     for (; it != mMap.end(); it++)
//     {
//         if (it->second.heapValue.get() == val)
//             return it->first;
//     }

    if (autoAdd)
    {
        JS::RootedValue rval(g_cx, val);
        return valueMap::add(rval, 10);
    }
	else
	{
		return containsValue(val);
	}
    return 0;
}


void valueMap::_clearTempIDs()
{
	std::list<int>::iterator it = LstTempID.begin();
	for (; it != LstTempID.end(); it++)
	{
		removeByID(*it, false);
	}
	LstTempID.clear();

// 	bool bStat = false;
// 	if (bStat)
// 	{
// 		std::map<int, int> m;
// 		for(VALUEMAPIT it = mMap.begin(); it != mMap.end(); it++)
// 		{
// 			m[it->second.mark] ++;
// 		}
// 		bStat = false;
// 	}
}


//     VALUEMAPIT it = mMap.find(iMap);
//     if (it != mMap.end())
//     {
//         // JS::RootedValue val(g_cx, it->second->heapValue.get());
//         valueArr::add(iArr, iMap);
//         return true;
//     }
//     return false;


// 
// std::map<int, JS::Value*> valueRoot::mMap;
// int valueRoot::index = 1;
// int valueRoot::add(JS::HandleValue val)
// {
//     JS::Value* pVal = new JS::Value;
//     *pVal = val;
//     bool ret = JS_AddValueRoot(g_cx, pVal);
//     mMap[index] = pVal;
//     return index++;
// }
// bool valueRoot::remove(int i)
// {
//     std::map<int, JS::Value*>::iterator it = mMap.find(i);
//     if (it != mMap.end())
//     {
//         JS::Value* pVal = it->second;
//         JS_RemoveValueRoot(g_cx, pVal);
//         delete pVal;
//         mMap.erase(it);
//         return true;
//     }
//     return false;
// }

