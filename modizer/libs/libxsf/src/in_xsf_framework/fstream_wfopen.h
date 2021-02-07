// This comes from llvm's libcxx project. I've copied the code from there (with heavy modifications) for use with MinGW to support opening Windows "Unicode" filenames.

#if defined(_WIN32) && !defined(_MSC_VER)
#pragma once

#include <streambuf>
#include <locale>
#include <memory>
#include <fstream>

class filebuf_wfopen : public std::filebuf
{
public:
	typedef typename traits_type::state_type state_type;

	// 27.9.1.2 Constructors/destructor:
	filebuf_wfopen() : __extbuf_(nullptr), __extbufnext_(nullptr), __extbufend_(nullptr), __ebs_(0),
		__intbuf_(nullptr), __ibs_(0), __file_(nullptr), __cv_(nullptr), __st_(), __st_last_(),
		__om_(static_cast<std::ios_base::openmode>(0)), __cm_(static_cast<std::ios_base::openmode>(0)),
		__owns_eb_(false), __owns_ib_(false), __always_noconv_(false)
	{
		if (std::has_facet<std::codecvt<char, char, state_type>>(this->getloc()))
		{
			this->__cv_ = &std::use_facet<std::codecvt<char, char, state_type>>(this->getloc());
			this->__always_noconv_ = this->__cv_->always_noconv();
		}
		this->setbuf(nullptr, 4096);
	}

	virtual ~filebuf_wfopen()
	{
		try
		{
			this->close();
		}
		catch (...)
		{
		}
		if (this->__owns_eb_)
			delete[] this->__extbuf_;
		if (this->__owns_ib_)
			delete[] this->__intbuf_;
	}

	// 27.9.1.4 Members:
	bool is_open() const
	{
		return !!this->__file_;
	}
	filebuf_wfopen *open(const char *__s, std::ios_base::openmode __mode)
	{
		filebuf_wfopen *__rt = nullptr;
		if (!this->__file_)
		{
			__rt = this;
			const char *__mdstr = nullptr;
			switch (__mode & ~std::ios_base::ate)
			{
				case std::ios_base::out:
				case std::ios_base::out | std::ios_base::trunc:
					__mdstr = "w";
					break;
				case std::ios_base::out | std::ios_base::app:
				case std::ios_base::app:
					__mdstr = "a";
					break;
				case std::ios_base::in:
					__mdstr = "r";
					break;
				case std::ios_base::in | std::ios_base::out:
					__mdstr = "r+";
					break;
				case std::ios_base::in | std::ios_base::out | std::ios_base::trunc:
					__mdstr = "w+";
					break;
				case std::ios_base::in | std::ios_base::out | std::ios_base::app:
				case std::ios_base::in | std::ios_base::app:
					__mdstr = "a+";
					break;
				case std::ios_base::out | std::ios_base::binary:
				case std::ios_base::out | std::ios_base::trunc | std::ios_base::binary:
					__mdstr = "wb";
					break;
				case std::ios_base::out | std::ios_base::app | std::ios_base::binary:
				case std::ios_base::app | std::ios_base::binary:
					__mdstr = "ab";
					break;
				case std::ios_base::in | std::ios_base::binary:
					__mdstr = "rb";
					break;
				case std::ios_base::in | std::ios_base::out | std::ios_base::binary:
					__mdstr = "r+b";
					break;
				case std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios_base::binary:
					__mdstr = "w+b";
					break;
				case std::ios_base::in | std::ios_base::out | std::ios_base::app | std::ios_base::binary:
				case std::ios_base::in | std::ios_base::app | std::ios_base::binary:
					__mdstr = "a+b";
					break;
				default:
					__rt = nullptr;
			}
			if (__rt)
			{
				this->__file_ = fopen(__s, __mdstr);
				if (this->__file_)
				{
					this->__om_ = __mode;
					if ((__mode & std::ios_base::ate) && fseek(this->__file_, 0, SEEK_END))
					{
						fclose(this->__file_);
						this->__file_ = nullptr;
						__rt = nullptr;
					}
				}
				else
					__rt = nullptr;
			}
		}
		return __rt;
	}
	filebuf_wfopen *open(const std::string &__s, std::ios_base::openmode __mode)
	{
		return this->open(__s.c_str(), __mode);
	}
	filebuf_wfopen *open(const wchar_t *__s, std::ios_base::openmode __mode)
	{
		filebuf_wfopen *__rt = nullptr;
		if (!this->__file_)
		{
			__rt = this;
			const wchar_t *__mdstr = nullptr;
			switch (__mode & ~std::ios_base::ate)
			{
				case std::ios_base::out:
				case std::ios_base::out | std::ios_base::trunc:
					__mdstr = L"w";
					break;
				case std::ios_base::out | std::ios_base::app:
				case std::ios_base::app:
					__mdstr = L"a";
					break;
				case std::ios_base::in:
					__mdstr = L"r";
					break;
				case std::ios_base::in | std::ios_base::out:
					__mdstr = L"r+";
					break;
				case std::ios_base::in | std::ios_base::out | std::ios_base::trunc:
					__mdstr = L"w+";
					break;
				case std::ios_base::in | std::ios_base::out | std::ios_base::app:
				case std::ios_base::in | std::ios_base::app:
					__mdstr = L"a+";
					break;
				case std::ios_base::out | std::ios_base::binary:
				case std::ios_base::out | std::ios_base::trunc | std::ios_base::binary:
					__mdstr = L"wb";
					break;
				case std::ios_base::out | std::ios_base::app | std::ios_base::binary:
				case std::ios_base::app | std::ios_base::binary:
					__mdstr = L"ab";
					break;
				case std::ios_base::in | std::ios_base::binary:
					__mdstr = L"rb";
					break;
				case std::ios_base::in | std::ios_base::out | std::ios_base::binary:
					__mdstr = L"r+b";
					break;
				case std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios_base::binary:
					__mdstr = L"w+b";
					break;
				case std::ios_base::in | std::ios_base::out | std::ios_base::app | std::ios_base::binary:
				case std::ios_base::in | std::ios_base::app | std::ios_base::binary:
					__mdstr = L"a+b";
					break;
				default:
					__rt = nullptr;
			}
			if (__rt)
			{
				this->__file_ = _wfopen(__s, __mdstr);
				if (this->__file_)
				{
					this->__om_ = __mode;
					if ((__mode & std::ios_base::ate) && fseek(this->__file_, 0, SEEK_END))
					{
						fclose(this->__file_);
						this->__file_ = nullptr;
						__rt = nullptr;
					}
				}
				else
					__rt = nullptr;
			}
		}
		return __rt;
	}
	filebuf_wfopen *open(const std::wstring &__s, std::ios_base::openmode __mode)
	{
		return this->open(__s.c_str(), __mode);
	}
	filebuf_wfopen *close()
	{
		filebuf_wfopen *__rt = nullptr;
		if (this->__file_)
		{
			__rt = this;
			std::unique_ptr<FILE, int (*)(FILE *)> __h(this->__file_, fclose);
			if (this->sync())
				__rt = nullptr;
			if (!fclose(__h.release()))
				this->__file_ = nullptr;
			else
				__rt = nullptr;
		}
		return __rt;
	}

protected:
	// 27.9.1.5 Overridden virtual functions:
	virtual int_type underflow()
	{
		if (!this->__file_)
			return traits_type::eof();
		bool __initial = this->__read_mode();
		char __1buf;
		if (!this->gptr())
			this->setg(&__1buf, &__1buf + 1, &__1buf + 1);
		const size_t __unget_sz = __initial ? 0 : std::min<size_t>((this->egptr() - this->eback()) / 2, 4);
		int_type __c = traits_type::eof();
		if (this->gptr() == this->egptr())
		{
			memmove(this->eback(), this->egptr() - __unget_sz, __unget_sz);
			if (this->__always_noconv_)
			{
				size_t __nmemb = static_cast<size_t>(this->egptr() - this->eback() - __unget_sz);
				__nmemb = fread(this->eback() + __unget_sz, 1, __nmemb, this->__file_);
				if (__nmemb)
				{
					this->setg(this->eback(), this->eback() + __unget_sz, this->eback() + __unget_sz + __nmemb);
					__c = traits_type::to_int_type(*this->gptr());
				}
			}
			else
			{
				memmove(this->__extbuf_, this->__extbufnext_, this->__extbufend_ - this->__extbufnext_);
				this->__extbufnext_ = this->__extbuf_ + (this->__extbufend_ - this->__extbufnext_);
				this->__extbufend_ = this->__extbuf_ +
					(this->__extbuf_ == this->__extbuf_min_ ? sizeof(this->__extbuf_min_) : this->__ebs_);
				size_t __nmemb = std::min<size_t>(this->__ibs_ - __unget_sz,
					this->__extbufend_ - this->__extbufnext_);
				std::codecvt_base::result __r;
				this->__st_last_ = this->__st_;
				size_t __nr = fread(const_cast<char *>(this->__extbufnext_), 1, __nmemb, this->__file_);
				if (__nr)
				{
					if (!this->__cv_)
						throw std::bad_cast();
					this->__extbufend_ = this->__extbufnext_ + __nr;
					char *__inext;
					__r = this->__cv_->in(this->__st_, this->__extbuf_, this->__extbufend_, this->__extbufnext_,
						this->eback() + __unget_sz, this->eback() + this->__ibs_, __inext);
					if (__r == std::codecvt_base::noconv)
					{
						this->setg(this->__extbuf_, this->__extbuf_, const_cast<char *>(this->__extbufend_));
						__c = traits_type::to_int_type(*this->gptr());
					}
					else if (__inext != this->eback() + __unget_sz)
					{
						this->setg(this->eback(), this->eback() + __unget_sz, __inext);
						__c = traits_type::to_int_type(*this->gptr());
					}
				}
			}
		}
		else
			__c = traits_type::to_int_type(*this->gptr());
		if (this->eback() == &__1buf)
			this->setg(nullptr, nullptr, nullptr);
		return __c;
	}
	virtual int_type pbackfail(int_type __c = traits_type::eof())
	{
		if (this->__file_ && this->eback() < this->gptr())
		{
			if (traits_type::eq_int_type(__c, traits_type::eof()))
			{
				this->gbump(-1);
				return traits_type::not_eof(__c);
			}
			if ((this->__om_ & std::ios_base::out) ||
				traits_type::eq(traits_type::to_char_type(__c), this->gptr()[-1]))
			{
				this->gbump(-1);
				*this->gptr() = traits_type::to_char_type(__c);
				return __c;
			}
		}
		return traits_type::eof();
	}
	virtual int_type overflow (int_type __c = traits_type::eof())
	{
		if (!this->__file_)
			return traits_type::eof();
		this->__write_mode();
		char __1buf;
		char *__pb_save = this->pbase();
		char *__epb_save = this->epptr();
		if (!traits_type::eq_int_type(__c, traits_type::eof()))
		{
			if (!this->pptr())
				this->setp(&__1buf, &__1buf + 1);
			*this->pptr() = traits_type::to_char_type(__c);
			this->pbump(1);
		}
		if (this->pptr() != this->pbase())
		{
			if (this->__always_noconv_)
			{
				size_t __nmemb = static_cast<size_t>(this->pptr() - this->pbase());
				if (fwrite(this->pbase(), 1, __nmemb, __file_) != __nmemb)
					return traits_type::eof();
			}
			else
			{
				char *__extbe = this->__extbuf_;
				std::codecvt_base::result __r;
				do
				{
					if (!this->__cv_)
						throw std::bad_cast();
					const char *__e;
					__r = this->__cv_->out(this->__st_, this->pbase(), this->pptr(), __e, this->__extbuf_,
						this->__extbuf_ + this->__ebs_, __extbe);
					if (__e == this->pbase())
						return traits_type::eof();
					if (__r == std::codecvt_base::noconv)
					{
						size_t __nmemb = static_cast<size_t>(this->pptr() - this->pbase());
						if (fwrite(this->pbase(), 1, __nmemb, this->__file_) != __nmemb)
							return traits_type::eof();
					}
					else if (__r == std::codecvt_base::ok || __r == std::codecvt_base::partial)
					{
						size_t __nmemb = static_cast<size_t>(__extbe - this->__extbuf_);
						if (fwrite(this->__extbuf_, 1, __nmemb, this->__file_) != __nmemb)
							return traits_type::eof();
						if (__r == std::codecvt_base::partial)
						{
							this->setp(const_cast<char *>(__e), this->pptr());
							this->pbump(this->epptr() - this->pbase());
						}
					}
					else
						return traits_type::eof();
				} while (__r == std::codecvt_base::partial);
			}
			this->setp(__pb_save, __epb_save);
		}
		return traits_type::not_eof(__c);
	}
	virtual std::streambuf *setbuf(char *__s, std::streamsize __n)
	{
		this->setg(nullptr, nullptr, nullptr);
		this->setp(nullptr, nullptr);
		if (this->__owns_eb_)
			delete[] this->__extbuf_;
		if (this->__owns_ib_)
			delete[] this->__intbuf_;
		this->__ebs_ = __n;
		if (this->__ebs_ > sizeof(this->__extbuf_min_))
		{
			if (this->__always_noconv_ && __s)
			{
				this->__extbuf_ = __s;
				this->__owns_eb_ = false;
			}
			else
			{
				this->__extbuf_ = new char[this->__ebs_];
				this->__owns_eb_ = true;
			}
		}
		else
		{
			this->__extbuf_ = this->__extbuf_min_;
			this->__ebs_ = sizeof(this->__extbuf_min_);
			this->__owns_eb_ = false;
		}
		if (!this->__always_noconv_)
		{
			this->__ibs_ = std::max<std::streamsize>(__n, sizeof(this->__extbuf_min_));
			if (__s && this->__ibs_ >= sizeof(this->__extbuf_min_))
			{
				this->__intbuf_ = __s;
				this->__owns_ib_ = false;
			}
			else
			{
				this->__intbuf_ = new char[this->__ibs_];
				this->__owns_ib_ = true;
			}
		}
		else
		{
			this->__ibs_ = 0;
			this->__intbuf_ = nullptr;
			this->__owns_ib_ = false;
		}
		return this;
	}
	virtual pos_type seekoff(off_type __off, std::ios_base::seekdir __way,
		std::ios_base::openmode = std::ios_base::in | std::ios_base::out)
	{
		if (!this->__cv_)
			throw std::bad_cast();
		int __width = this->__cv_->encoding();
		if (!this->__file_ || (__width <= 0 && __off) || this->sync())
			return pos_type(off_type(-1));
		// __width > 0 || __off == 0
		int __whence;
		switch (__way)
		{
			case std::ios_base::beg:
				__whence = SEEK_SET;
				break;
			case std::ios_base::cur:
				__whence = SEEK_CUR;
				break;
			case std::ios_base::end:
				__whence = SEEK_END;
				break;
			default:
				return pos_type(off_type(-1));
		}
		if (fseek(this->__file_, __width > 0 ? __width * __off : 0, __whence))
			return pos_type(off_type(-1));
		pos_type __r = ftell(this->__file_);
		__r.state(this->__st_);
		return __r;
	}
	virtual pos_type seekpos(pos_type __sp,
		std::ios_base::openmode = std::ios_base::in | std::ios_base::out)
	{
		if (!this->__file_ || this->sync())
			return pos_type(off_type(-1));
		if (fseek(this->__file_, __sp, SEEK_SET))
			return pos_type(off_type(-1));
		this->__st_ = __sp.state();
		return __sp;
	}
	virtual int sync()
	{
		if (!this->__file_)
			return 0;
		if (!this->__cv_)
			throw std::bad_cast();
		if (this->__cm_ & std::ios_base::out)
		{
			if (this->pptr() != this->pbase() && this->overflow() == traits_type::eof())
				return -1;
			std::codecvt_base::result __r;
			do
			{
				char *__extbe;
				__r = this->__cv_->unshift(this->__st_, this->__extbuf_, this->__extbuf_ + this->__ebs_,
					__extbe);
				size_t __nmemb = static_cast<size_t>(__extbe - this->__extbuf_);
				if (fwrite(this->__extbuf_, 1, __nmemb, this->__file_) != __nmemb)
					return -1;
			} while (__r == std::codecvt_base::partial);
			if (__r == std::codecvt_base::error)
				return -1;
			if (fflush(this->__file_))
				return -1;
		}
		else if (this->__cm_ & std::ios_base::in)
		{
			off_type __c;
			state_type __state = this->__st_last_;
			bool __update_st = false;
			if (this->__always_noconv_)
				__c = this->egptr() - this->gptr();
			else
			{
				int __width = this->__cv_->encoding();
				__c = this->__extbufend_ - this->__extbufnext_;
				if (__width > 0)
					__c += __width * (this->egptr() - this->gptr());
				else if (this->gptr() != this->egptr())
				{
					int __off =  this->__cv_->length(__state, this->__extbuf_, this->__extbufnext_,
						this->gptr() - this->eback());
					__c += this->__extbufnext_ - this->__extbuf_ - __off;
					__update_st = true;
				}
			}
			if (fseek(this->__file_, -__c, SEEK_CUR))
				return -1;
			if (__update_st)
				this->__st_ = __state;
			this->__extbufnext_ = this->__extbufend_ = this->__extbuf_;
			this->setg(nullptr, nullptr, nullptr);
			this->__cm_ = static_cast<std::ios_base::openmode>(0);
		}
		return 0;
	}
	virtual void imbue(const std::locale &__loc)
	{
		this->sync();
		this->__cv_ = &std::use_facet<std::codecvt<char, char, state_type>>(__loc);
		bool __old_anc = this->__always_noconv_;
		this->__always_noconv_ = this->__cv_->always_noconv();
		if (__old_anc != this->__always_noconv_)
		{
			this->setg(nullptr, nullptr, nullptr);
			this->setp(nullptr, nullptr);
			// invariant, char_type is char, else we couldn't get here
			if (this->__always_noconv_) // need to dump __intbuf_
			{
				if (this->__owns_eb_)
					delete[] this->__extbuf_;
				this->__owns_eb_ = this->__owns_ib_;
				this->__ebs_ = this->__ibs_;
				this->__extbuf_ = this->__intbuf_;
				this->__ibs_ = 0;
				this->__intbuf_ = nullptr;
				this->__owns_ib_ = false;
			}
			// need to obtain an __intbuf_.
			else if (!this->__owns_eb_ && this->__extbuf_ != this->__extbuf_min_)
				// If __extbuf_ is user-supplied, use it, else new __intbuf_
			{
				this->__ibs_ = this->__ebs_;
				this->__intbuf_ = this->__extbuf_;
				this->__owns_ib_ = false;
				this->__extbuf_ = new char[this->__ebs_];
				this->__owns_eb_ = true;
			}
			else
			{
				this->__ibs_ = this->__ebs_;
				this->__intbuf_ = new char[this->__ibs_];
				this->__owns_ib_ = true;
			}
		}
	}

private:
	char *__extbuf_;
	const char *__extbufnext_;
	const char *__extbufend_;
	char __extbuf_min_[8];
	size_t __ebs_;
	char *__intbuf_;
	size_t __ibs_;
	FILE *__file_;
	const std::codecvt<char, char, state_type> *__cv_;
	state_type __st_;
	state_type __st_last_;
	std::ios_base::openmode __om_;
	std::ios_base::openmode __cm_;
	bool __owns_eb_;
	bool __owns_ib_;
	bool __always_noconv_;

	bool __read_mode()
	{
		if (!(this->__cm_ & std::ios_base::in))
		{
			this->setp(nullptr, nullptr);
			if (this->__always_noconv_)
				this->setg(this->__extbuf_, this->__extbuf_ + this->__ebs_, this->__extbuf_ + this->__ebs_);
			else
				this->setg(this->__intbuf_, this->__intbuf_ + this->__ibs_, this->__intbuf_ + this->__ibs_);
			this->__cm_ = std::ios_base::in;
			return true;
		}
		return false;
	}
	void __write_mode()
	{
		if (!(this->__cm_ & std::ios_base::out))
		{
			this->setg(nullptr, nullptr, nullptr);
			if (this->__ebs_ > sizeof(this->__extbuf_min_))
			{
				if (this->__always_noconv_)
					this->setp(this->__extbuf_, this->__extbuf_ + (this->__ebs_ - 1));
				else
					this->setp(this->__intbuf_, this->__intbuf_ + (this->__ibs_ - 1));
			}
			else
				this->setp(nullptr, nullptr);
			this->__cm_ = std::ios_base::out;
		}
	}
};

// basic_ifstream

class ifstream_wfopen : public std::ifstream
{
public:
	ifstream_wfopen() { std::ios::rdbuf(&this->__sb_); }
	explicit ifstream_wfopen(const char *__s, std::ios_base::openmode __mode = std::ios_base::in)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::in))
			std::ios::rdbuf(&this->__sb_);
		else
			this->setstate(std::ios_base::failbit);
	}
	explicit ifstream_wfopen(const std::string &__s, std::ios_base::openmode __mode = std::ios_base::in)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::in))
			std::ios::rdbuf(&this->__sb_);
		else
			this->setstate(std::ios_base::failbit);
	}
	explicit ifstream_wfopen(const wchar_t *__s, std::ios_base::openmode __mode = std::ios_base::in)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::in))
			std::ios::rdbuf(&this->__sb_);
		else
			this->setstate(std::ios_base::failbit);
	}
	explicit ifstream_wfopen(const std::wstring &__s, std::ios_base::openmode __mode = std::ios_base::in)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::in))
			std::ios::rdbuf(&this->__sb_);
		else
			this->setstate(std::ios_base::failbit);
	}

	std::filebuf *rdbuf() const
	{
		return const_cast<std::filebuf *>(static_cast<const std::filebuf *>(&this->__sb_));
	}
	bool is_open() const
	{
		return this->__sb_.is_open();
	}
	void open(const char *__s, std::ios_base::openmode __mode = std::ios_base::in)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::in))
			this->clear();
		else
			this->setstate(std::ios_base::failbit);
	}
	void open(const std::string &__s, std::ios_base::openmode __mode = std::ios_base::in)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::in))
			this->clear();
		else
			this->setstate(std::ios_base::failbit);
	}
	void open(const wchar_t *__s, std::ios_base::openmode __mode = std::ios_base::in)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::in))
			this->clear();
		else
			this->setstate(std::ios_base::failbit);
	}
	void open(const std::wstring &__s, std::ios_base::openmode __mode = std::ios_base::in)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::in))
			this->clear();
		else
			this->setstate(std::ios_base::failbit);
	}
	void close()
	{
		if (!this->__sb_.close())
			this->setstate(std::ios_base::failbit);
	}

private:
	filebuf_wfopen __sb_;
};

// basic_ofstream

class ofstream_wfopen : public std::ofstream
{
public:
	ofstream_wfopen() { std::ios::rdbuf(&this->__sb_); }
	explicit ofstream_wfopen(const char *__s, std::ios_base::openmode __mode = std::ios_base::out)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::out))
			std::ios::rdbuf(&this->__sb_);
		else
			this->setstate(std::ios_base::failbit);
	}
	explicit ofstream_wfopen(const std::string &__s, std::ios_base::openmode __mode = std::ios_base::out)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::out))
			std::ios::rdbuf(&this->__sb_);
		else
			this->setstate(std::ios_base::failbit);
	}
	explicit ofstream_wfopen(const wchar_t *__s, std::ios_base::openmode __mode = std::ios_base::out)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::out))
			std::ios::rdbuf(&this->__sb_);
		else
			this->setstate(std::ios_base::failbit);
	}
	explicit ofstream_wfopen(const std::wstring &__s, std::ios_base::openmode __mode = std::ios_base::out)
	{
		if (!this->__sb_.open(__s, __mode | std::ios_base::out))
			std::ios::rdbuf(&this->__sb_);
		else
			this->setstate(std::ios_base::failbit);
	}

	std::filebuf *rdbuf() const
	{
		return const_cast<std::filebuf *>(static_cast<const std::filebuf *>(&this->__sb_));
	}
	bool is_open() const
	{
		return this->__sb_.is_open();
	}
	void open(const char *__s, std::ios_base::openmode __mode = std::ios_base::out)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::out))
			this->clear();
		else
			this->setstate(std::ios_base::failbit);
	}
	void open(const std::string &__s, std::ios_base::openmode __mode = std::ios_base::out)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::out))
			this->clear();
		else
			this->setstate(std::ios_base::failbit);
	}
	void open(const wchar_t *__s, std::ios_base::openmode __mode = std::ios_base::out)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::out))
			this->clear();
		else
			this->setstate(std::ios_base::failbit);
	}
	void open(const std::wstring &__s, std::ios_base::openmode __mode = std::ios_base::out)
	{
		if (this->__sb_.open(__s, __mode | std::ios_base::out))
			this->clear();
		else
			this->setstate(std::ios_base::failbit);
	}
	void close()
	{
		if (!this->__sb_.close())
			this->setstate(std::ios_base::failbit);
	}

private:
	filebuf_wfopen __sb_;
};
#endif
