/*
 HeapSkew.h: A skew heap implementation
 Copyright 1999 G. Andrew Stone
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#if !defined(HeapSkewH)
#define HeapSkewH

//#include "gen.hxx"
#define assert(x) 

  // T must have a > comparison operator.
  // T must have a SkewHeapElem class with the variable name skewChildren; i.e. "HeapSkew<T>::SkewHeapElem  skewChildren;"
template<class T> class HeapSkew
  {
  public:

  void push  (T& t);            // Add A new element onto the heap.
  T&   pop   (void);            // Remove the top element of the heap.
  T&   front (void);            // Return the top element of the heap.
  void merge (HeapSkew& h);     // All elements of 'h will be merged into 'this.  So 'h will end empty.

  T&   clear (void);            // Remove all elements, return the root of the tree of removed nodes.

  HeapSkew():mRoot(0) {}

  ~HeapSkew() {}

  class SkewHeapElem
    {
    public:
    SkewHeapElem(): Left(0),Right(0) {}
    T* Left;
    T* Right;
    };

    // Statistics
  unsigned int DepthLastOp;
  unsigned int MaxDepth;
    

  private:

  T* merge (T* a, T* b);  

  void PtrSwap(T **a,T **b)
    {
      T *c;
      c    = (*a);
      (*a) = (*b);
      (*b) = c;
  }
      

  T* mRoot;
  };


template<class T> void HeapSkew<T>::push(T& t)
  {
  assert(&t);
  t.skewChildren.Left = t.skewChildren.Right = 0;
  mRoot = merge(&t,mRoot);
  }

template<class T> T& HeapSkew<T>::pop   (void)
  {
  T& ret = *mRoot;
  if (mRoot) mRoot = merge(mRoot->skewChildren.Left, mRoot->skewChildren.Right);
  if (&ret) ret.skewChildren.Left = ret.skewChildren.Right = 0;
  return ret;
  }

template<class T> T& HeapSkew<T>::front  (void)
  {
  return *mRoot;
  }

template<class T> T& HeapSkew<T>::clear (void)
  {
  T* ret = mRoot;
  mRoot  = NULL;
  return *ret;  
  }


template<class T> void HeapSkew<T>::merge (HeapSkew& h)
  {
  merge(h.mRoot);
  h.mRoot = 0;
  }


template<class T> T* HeapSkew<T>::merge (T* a, T* b)
  {
  byte SizeCtr=0;
  T *Ret;
  DepthLastOp = 0;
  if (a==0) return b;
  if (b==0) return a;

  if (*a > *b) PtrSwap(&a,&b);  /* Make a the smallest. */

  Ret=a;

  while(b!=0)
    {
    if (a->skewChildren.Right == 0) { a->skewChildren.Right=b; break; }
    if (*a->skewChildren.Right > *b) PtrSwap(&a->skewChildren.Right,&b);
    PtrSwap(&a->skewChildren.Right,&a->skewChildren.Left);
    a=a->skewChildren.Left; /* Really the right, but I just swapped them. */
    SizeCtr++;
    }
  
  DepthLastOp = SizeCtr;
  if (SizeCtr > MaxDepth) MaxDepth = SizeCtr;
  //  if (Count) AveHeapDepth = (99.0*AveHeapDepth+((float)SizeCtr))/100.0;
  return(Ret);
  }



template<class T> class HeapSkewMemMgmt
{
  
  class Wrapper
  {
  public:
    typename HeapSkew<Wrapper>::SkewHeapElem   skewChildren; 
    T                                    userData;
    Wrapper(const T& data): userData(data) {}
    int operator > (const Wrapper& w) { return (userData > w.userData); }
  };

  HeapSkew<Wrapper> sk;

public:

  void push  (const T& t)
  {
    sk.push(*(new Wrapper(t)));
  }

  T   pop   (void) 
  {
    Wrapper* w = &sk.pop();
    T ret = w->userData;
    delete w;
    return ret;
  }

  T&   front (void)       
  {
    Wrapper* w = &sk.front();
    return w->userData;
  }

  void merge (HeapSkewMemMgmt& h)
  {
    sk.merge(h.sk);
  }

};



#endif
