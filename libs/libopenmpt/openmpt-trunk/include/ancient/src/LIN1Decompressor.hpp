/* Copyright (C) Teemu Suutari */

#ifndef LIN1DECOMPRESSOR_HPP
#define LIN1DECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

namespace ancient::internal
{

class LIN1Decompressor : public XPKDecompressor
{
public:
	LIN1Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::shared_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~LIN1Decompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::shared_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::shared_ptr<XPKDecompressor::State> &state,bool verify);

private:
	const Buffer	&_packedData;

	uint32_t	_ver=0;
};

}

#endif
