
// g++ -o constexpr.exe constexpr.cpp -static-libstdc++ -std=c++17 -ID:\Code\boost_1_66_0

// MinGW was taking libstd++6.dll from Windows instead of the one in MinGW
// -std=c++17 for fold expressions
// -ID:\Code\boost_1_66_0 to include and use Boost code (just the template/header ones, need to link to libs for some Boost stuff, for that use -L )


// C++ 17 : constexpr if, fold expressions, init statement for if/switch, a lot of the make_pair,make_tuple etc std functions you can stop using because of template argument deduction so use normal constructors instead

#include<iostream> 
#include<chrono> 
#include<functional> 
#include <typeinfo>
#include <memory>
#include <vector>
#include <boost\core\demangle.hpp>
#include <cstring>
#include <thread>
#include <future>
#include <mutex>

using namespace std; 
  
long int fib(int n) 
{ 
    return (n <= 1)? n : fib(n-1) + fib(n-2); 
} 

constexpr long int fibCE(int n) 
{ 
    return (n <= 1)? n : fibCE(n-1) + fibCE(n-2); 
} 


// Instantiate a whole array of constexprs
// Here, the whole instantiation in the constructor is done at compile time!
template<int N>
struct A { // using a class and setting all to public works just as well
    constexpr A() { // : arr() needed because it needs to be initialized for constexpr to work. Note: removed because initing to {0} below works as well
        for (auto i = 0; i != N; ++i)
            arr[i] = fibCE(i + 37); // note, it is not allowed to call non-const-expr function from here (like fib instead of fibCE)
    }
    int arr[N] = {0};
};



template <typename... T>
auto sum(T ... args) {
	return (... + args); // (1), all work here, difference is how they unfold
	//return (0 + ... + args); // (2)
	//return (args + ...); // (3)
	//return (args + ... + 0); // (4)
}

/*
Fold expression unpack types:
1. (... op pack) 			e.g. "... + args"
2. (init op ... op pack)    e.g. "cout << .. << args
3. (pack op ...)  			e.g. "args + ..."
4. (pack op ... op init)	e.g. "args + ... + 0"
*/

template<typename ...T>
void FoldPrint(T&& ... args) {
	(cout << ... << args) << "\n"; // (init op ... op pack)
//	(cout << args << ...) << "\n"; // Nope, cause there is no unpack like this: (init op pack op ...)
//	(cout << ... << std::forward<T>(args)) << "\n"; // supposedly this is "better". Both "works".   *In a nutshell, forward preserves the value category of its argument. Perfect forwarding is there to ensure that the argument provided to a function is forwarded to another function (or used within the function) with the same value category (basically r-value vs. l-value) as originally provided. 
}

template <typename T, typename... Args>
void push_back_vec(vector<T> &v, Args&& ...args)
{
	(v.push_back(args) , ...); // comma separator makes e.g.: v.push_back(arg0),v.push_back(arg1),v.push_back(arg2);
}


struct foo {
    explicit foo(int i) {}
    foo(int i, int j) {}
    explicit foo(int i, int j, int k) {}
};


foo make_foo()
{
	// This func is to demonstrate use if explicit for constructors. Not though that he bracket syntax makes it possible to return a foo using only "return { 42 ,42 };", no need for "return foo(42,42);" 
	// Also this func should use RVO (move semantics), so it's good, don't return by ref

    return { 42, 42 };
	
    /* Not C++11-specific: */
    // Error: no conversion from int to foo
    //return 42;

    // Okay: construction, not conversion
    return foo(42);

    // Okay: constructions
    return foo(42, 42);
    return foo(42, 42, 42);

    /* C++11 and higher specific: */
    // Error: no conversion from int to foo
    //return { 42 };

    // Not an error, not a conversion
    return { 42, 42 };

    // Error! Constructor is explicit
    //return { 42, 42, 42 };
    // Not an error, direct-initialization syntax
    return foo { 42, 42, 42 };
}

/*
void fnoe() noexcept { // g++ compiles, but gives warning:  warning: throw will always call terminate(). Removing noexcept removes this warning. Not removing itand running the func terminates the app with a special message
	throw 1;
} */

__forceinline void fnoe() noexcept { // no more warning since we always catch exception
	try {
//		throw 1;
		throw std::string("Err!"); // we can throw/catch pretty much anything in C++ but recommended is to throw derived class of std::exception
//	} catch(int a) { cout << "Caught: " << a << "\n"; }
	} catch(std::string s) { cout << "Caught: " << s << "\n"; }
	//} catch(std::exception e) { cout << "Caught it!\n"; } // this line doesn't catch the int exception, so program terminates. Warning does not show though
}

int main () 
{ 
	auto timerFunc = [](auto f, std::function<void (long long)> out = nullptr) { // think I must use std::function here for arg2, since I wanted it to have nullptr as default value
		using namespace std::chrono; // for the clocks, microseconds, duration_cast
		auto start = high_resolution_clock::now();

		f();

		auto stop = high_resolution_clock::now();
		auto diff = duration_cast<microseconds>(stop - start);
		
		if (out != nullptr)
			out(diff.count());
		else
			cout << "Time spent: " << diff.count() << " ms\n";
	};

	auto printTimer = [](long long timeSpent) { cout << "That took: " << timeSpent << " MICRO seconds (not milli)\n"; };

    long int res = fib(35);
	timerFunc([&]() { res = fib(35); }, printTimer );
    cout << res << "\n";

	timerFunc([&]() { constexpr long int tmp = fibCE(40); res = tmp; }, printTimer ); //Seems also for g++, in some cases result var must be tagged as constexpr too (like here) to be precomputed. So always do that!
    cout << res << "\n";

	// timerFunc([&]() { constexpr auto a = A<4>(); }, printTimer ); // ok, but how to use a outside of lambda?
	const int arrSize = 4;
	int arr[arrSize];
	timerFunc([&]() { constexpr auto tmp = A<arrSize>(); for(int i = 0; i < arrSize; i++) arr[i] = tmp.arr[i]; }, printTimer ); // one way, but not using struct A type for actual results (as they are created in constructor)
	for (auto x : arr)
        std::cout << x << '\n';

	cout << sum(45, 66, 88, 109) << "\n";

	FoldPrint("Hello ", "Mr ", 242, '!');
	
	if (0 and 5) { // and, bitor, or, xor, compl, bitand, and_eq, or_eq, xor_eq, not, and not_eq provide an alternative way to represent standard tokens
		cout << "Ain't appenin'\n";
	}
	
	auto f = [](int a) { return 66; };
	decltype(f) g = f; // g kommer här bli en std::function<int (int)>
	int n = 88;

	//cout << "n has type " << typeid(n).name() << '\n'; // ummm.. NOT what I expected, the names are all mangled: n has type i     g has type Z4mainEUliE4_   . Supposedly boost (core/demangle.hpp) can be used to "demangle"
	//cout << "g has type " << typeid(g).name() << '\n';

    cout << "n has type " << boost::core::demangle( typeid(n).name() ) << std::endl; // int,     main::{lambda(int)#6}    much better, still not great (return type for lambda not shown)
	cout << "g has type " << boost::core::demangle( typeid(g).name() ) << '\n';

	foo foooo = make_foo(); // shows use of explicit
	
	fnoe();
	cout << "Hehe...\n";

	const int arraySize = 1920 * 1080 * 10;
	
	// Note: Interesting, using no -O option, memset is CLEARLY the fastest! But using -O3, all four solutions are typically very similar in speed (fastest one varies)! 
	
	{
		auto unique_block = make_unique<unsigned char []>(arraySize);
		unsigned char *pBlock = unique_block.get();
		timerFunc([&]() { std::fill(pBlock, pBlock + arraySize, 0x77); }, printTimer);
	}
	{
		auto unique_block = make_unique<unsigned char []>(arraySize);
		unsigned char *pBlock = unique_block.get();
		timerFunc([&]() { memset(pBlock, 0x77, arraySize * sizeof(unsigned char)); }, printTimer);
	}
	{
		auto unique_block = make_unique<unsigned char []>(arraySize);
		unsigned char *pBlock = unique_block.get();
		timerFunc([&]() { for (int i = 0; i < arraySize; i++) *pBlock++ = 0x77; }, printTimer);
	}
	{
		auto unique_block = vector<unsigned char>(arraySize);
		timerFunc([&]() { std::fill(unique_block.begin(), unique_block.end(), 0x77); }, printTimer); // time-wise, pretty much the same
	}


	{
		auto unique_block = make_unique<unsigned char []>(arraySize);
		unsigned char *pBlock = unique_block.get();
		int i = 0;
		timerFunc([&]() { std::generate(pBlock, pBlock + arraySize, [&i]() { return (i++) % 256; } ); }, printTimer);
	}
	{
		auto unique_block = make_unique<unsigned char []>(arraySize);
		unsigned char *pBlock = unique_block.get();
		timerFunc([&]() { for (int i = 0; i < arraySize; i++) *pBlock++ = i % 256;; }, printTimer);
	}
	{
		auto unique_block = vector<unsigned char>(arraySize);
		int i = 0;
		timerFunc([&]() { std::generate(unique_block.begin(), unique_block.end(), [&i]() { return (i++) % 256; } ); }, printTimer); // time-wise, pretty much the same
	}


	// algorithm std functions: 
	
	// non-modifying:  (bool) all_of, any_of, none_of    for_each/for_each_n     count/count_if   mismatch(find first elem where TWO ranges differ)  find,find_if,find_if_not   search/search_n (look for RANGE inside range)
	//		    Also:  find_first_of(search for any of a set of elements),find_end(find the last seq of elem in range),adjacent_find(find first two items following each other that are equal or satisfy predicate)
	
	// modifying:  copy,copy_if,copy_n,copy_backward,move_backward (copy or move range to new location)   fill,fill_n   tranform(applies func to range and store in new range),  generate/generate_n(func to write to itself)
	//				remove/remove_if   remove_copy/replace/replace_copy  reverse/rotate   shuffle(randomly reorders)  sample(select randomly among range)  unique/unique_copy (remove duplicates in range)
	
	// partitioning (one range appears before second range): is_partitioned, partition, etc

	// sorting: is_sorted, sort, partial_sort, stable_sort
	//	also: binary_search
	
	// sorted range operations:
	//    merge. (bool)includes, set_difference,set_intersection,set_union,set_symmetric_difference
	
	// heap operations:?? permutations? (e.g. is_permutation)
	
	// min/max;  max/max_element/min/min_element/minmax/clamp
	
	// other:
	//	std::distance (for e.g. iterators),advance,insert_iterator,move_iterator,...          swap, iter_swap, iota, accumulate, inner_product, reduce etcetc
	
	// Optimization:  std::move to force use of && overload, _forceinline


	// Calling base class (if we have overridden it):  nameofbaseclass::method()
	// c++ does not have a super or base keyword (due to supporting multiple inheritance)  
	
	// #pragma once    : C++ way of #ifndef ME_H #define ME_H etc...

	mutex m;

	string sT = "";
	auto threadFunc = [&sT,&m](const char c, const int nof) { 
		for (int i = 0; i < nof; i++) {
			std::scoped_lock<std::mutex> lock(m); // unlocks after each loop
			sT = sT + c;
		}
	};

	thread t1(threadFunc, 'x', 200);
	thread t2(threadFunc, 'o', 200);
	
	t1.join();
	t2.join();
	
	cout << sT << "\n";
	
    return 0; 
} 


/*
C++ keywords
==============
This is a list of reserved keywords in C++. Since they are used by the language, these keywords are not available for re-definition or overloading.
https://en.cppreference.com/w/cpp/keyword

Alternative for standard tokens:
and: &&
and_eq: &=
bitand: &
or: ||
or_eq: |=
bitor: |
xor: ^
xor_eq: ^=
compl: ~
not: !
not_eq: !=


alignas (since C++11)		: exempel: struct alignas(8) S {};    Can be used on class/struct/enum and (most) variables. Specifies aligment requirement
alignof (since C++11)		: exempel alignof(Foo) or alignof(char).  Returns alignment requirements of type. If it has been manually set with alignas, then alignof returns this value
asm
atomic_cancel (TM TS)		: TMTS: transactional memory op(s), experimental feature. (Transactional memory is a concurrency synchronization mechanism combining groups of statements in transactions, using special HW if possible)
atomic_commit (TM TS)		: -|-
atomic_noexcept (TM TS)		: -|-
auto(1)
bool
break
case
catch
char
char16_t (since C++11)		: large enough to repr any UTF-16 code unit (what's the diff with wchar_t? Answer seems to be it's not reliable to use wchar_t because on Windows it's 16-bit and other systems it might be 32 bit)
char32_t (since C++11)		: large enough to repr any UTF-32 code unit 
class(1)
const
constexpr (since C++11)
const_cast					: add or remove const from variable. Highly unrecommended.
continue
decltype (since C++11)		: Inspects the declared type of an entity or the type and value category of an expression. Exempel: int i = 33; decltype(i) j = i * 2;   (även j blir här en int... antar detta kan vara användbart om man inte "vet" typen, t.ex en auto?). decltype is useful when declaring types that are difficult or impossible to declare using standard notation, like lambda-related types or types that depend on template parameters. 
default(1)					: switch default
delete(1)
do
double
dynamic_cast				: casta mellan klasser i en klass-hierarki, typiskt downcasta en klass till en *derived* class. (upcasta betyder däremot att man castar från en derived till base, men för detta behövs inte dynamic_cast, det är ok ändå).  Exempel: Base* b1 = new Base; if(Derived* d = dynamic_cast<Derived*>(b1)) { ... }
else
enum						: also, "enum class"
explicit					: Specifies that a constructor or conversion function (since C++11) or deduction guide (since C++17) is explicit, that is, it cannot be used for implicit conversions and copy-initialization. Simple example: class B {explicit B(int) { }}  Without explicit, C++ would allow writing e.g: B b = 1; since C++ would automatically call constructor B(int). But since we marked it explicit, this is not allowed by compiler. (The traditional wisdom is that constructors taking one parameter (explicitly or effectively through the use of default parameters) should be marked explicit). Prevent mistakenly doing a=b when we don't want that.
export(1)(3)				:unused and unreserved, up until C++20. Template thing.
extern(1)					:several diff uses. One is extern "C" in front of a function or several within {}, meaning they can be called from other modules written in C, since no name mangling for overloading occurs. Another is an extern unsigned char* g_video, which means we want to point to a global that was actually declared and defined in another file. Also several other uses.
false
float
for
friend						: an entire class can be a friend, or constructors/destructors, or a single method in another class, or for operator overloading such as <<. E.g.: friend std::ostream& operator<<(std::ostream& out, const Y& o);   friend char* X::foo(int);   friend X::X(char), X::~X();  // last is constr/destr friends
goto
if
inline(1)					:inline is a suggestion, not guaranteed. For small mthods it should usually happen though. A method/func completely defined in a header is implicit inline. Same is true for constexpr method/func
int
long
mutable(1)					:suggests that part of something that is const, is NOT const. Example: const struct { int n1; mutable int n2; } x = {0, 0};  x.n1=4; //error, x struct is const.  x.n2=4; // ok, n2 is mutable 
namespace
new
noexcept (since C++11)		: a method/func marked noexcept promises not to throw any (unhandled) exceptions. If it still DOES throw an exception, that seems to lead to an immediate termination. Examples: void f() noexcept; // the function f() does not throw     void (*fp)() noexcept(false); // fp points to a function that may throw       void g(void pfa() noexcept);  // g takes a pointer to function that doesn't throw
nullptr (since C++11)
operator
private
protected
public
reflexpr (reflection TS)	: experimental, no explanation/use yet. For reflection, obviously.
register(2)					: unused/reserved since C++17
reinterpret_cast			: a stronger cast than static_cast, since it can e.g. cast from one pointer type to another. Use with care. Cannot cast on/off const or volatile.
return
short
signed
sizeof(1)
static						: inside class=not part of class instance, inside function=keeps its value, initializes once, as part of function declaration=cannot be accessed/linked to from outside its own source file
static_assert (since C++11)	: Performs compile-time assertion checking. Useful for e.g. templates, example: template <class T> struct myS { static_assert(std::is_default_constructible<T>::value, "Dude, we won't allow that");};  struct no_default { no_default () = delete; };   data_structure<int> ds_ok;   myS<no_default> ds_error; // compile error here since we removed default constructor of no_default struct.    Note: there are heaps of type checkers like std::is_default_constructible<T>::value
static_cast					: the least powerful casting in C++, e.g. no cast between pointer types. Use whenever possible
struct(1)					: the only difference between a struct and a class in C++ is struct has default public accessability whereas class has private
switch
synchronized (TM TS)		: TMTS, see atomic_cancel above
template
this
thread_local (since C++11)	: put in front of an either global or func static variable to mean that each thread has its own copy of the variable, so it is safe to modify it (but the value can't be shared then of course) 
throw
true
try
typedef
typeid						: Queries information of a type. Used where the dynamic type of a polymorphic object must be known and for static type identification. E.g: int myint=50; cout << "Type: " << typeid(myint).name();
typename
union
unsigned
using(1)					: Many uses, like: using namespace std; (to include whole namespace), or using namespace std::chrono; (to only include Chrono), or using std::vector to specifically allow using vector keyword without std.  Or using namespace MyShit.  Or as an alias, a more C++ typedef, e.g: using UPtrMapSS = std::unique_ptr<std::unordered_map<std::string, std::string>>;  now can use UPtrMapSS instead of long name.
virtual						: A virtual method can be overriden. A virtual =0 method is "pure virtual" and creates an abstract class. If you inherit a class with the virtual keyword (instead of just private or public), it means that "grandchildren will not inherit twice"... umm.. basically a way to fix multiple inheritance issues. If both B and C derives from A, and then we make D inherit from BOTH B and C, then it will have two instances of A, including all its variables, methods etc, which is a mess to use cause you need to specify if it's B::A or C::A you use. But if B and C inherits virtual from A, then this problem goes away cause D only gets single A instance  
void
volatile					: a variable marked volatile says to compiler "do not optimize away this variable ever". A volatile is NOT thread safe by itself, use locks
wchar_t						: same as char16_t on Windows, but same as char32_t on other systems... Conclusion is to probably avoid this, unless coding for single platform only.
while

In addition to keywords, there are identifiers with special meaning, which may be used as names of objects or functions, but have special meaning in certain contexts.

final (C++11)				: put in after class/struct to make it impossible to derive from, e.g. struct A final {},  or struct B final : A {} to inherit but allow no further subclasses. Also, an implementation of an overriden method can be marked final to allow no further overrides, e.g. "void foo() final;" assuming we are overriding "virtual void foo();" in base class.  

override (C++11)			: you don't have to write this to override a virtual method in C++, BUT it makes the interface clearer, AND perhaps more importantly compiler will warn if base class method is NOT virtual (to avoid hiding). Also helps if we accidentally put e.g const somewhere on our overriding method which was not on base class, compiler will then complain it is NOT the same and we are NOT overriding.

transaction_safe (TM TS)	: TMTS, see atomic_cancel above
transaction_safe_dynamic (TM TS)

    (1) - meaning changed or new meaning added in C++11.
    (2) - meaning changed in C++17.
    (3) - meaning changed in C++20. 

Note that and, bitor, or, xor, compl, bitand, and_eq, or_eq, xor_eq, not, and not_eq (along with the digraphs <%, %>, <:, :>, %:, and %:%:) provide an alternative way to represent standard tokens.

Also, all identifiers that contain a double underscore __ in any position and each identifier that begins with an underscore followed by an uppercase letter is always reserved and all identifiers that begin with an underscore are reserved for use as names in the global namespace. See identifiers for more details. 

New C++ 20 keywords:
concept (since C++20)
consteval (since C++20)
constinit (since C++20)
requires (since C++20)
co_await (since C++20)
co_return (since C++20)
co_yield (since C++20)
char8_t (since C++20)
*/
