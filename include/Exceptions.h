#pragma once

#include <exception>  // exception
#include <string>  // string


class bad_record_info : public std::exception
{
public:
	virtual const char* what() const throw() override
	{
		return "Failed to get next record info!";
	}
};


class bad_record_version : public std::runtime_error
{
public:
	explicit bad_record_version(UInt32 a_versionExpected, UInt32 a_versionFound) :
		runtime_error("Record is out of date! Expected (" + std::to_string(a_versionExpected) + "), found (" + std::to_string(a_versionFound) + ")!")
	{}
};


class bad_record_read : public std::exception
{
public:
	virtual const char* what() const throw() override
	{
		return "Failed to read record data!";
	}
};


class bad_ammo_save : public std::exception
{
public:
	virtual const char* what() const throw() override
	{
		return "Failure while saving ammo!";
	}
};


class bad_helmet_save : public std::exception
{
public:
	virtual const char* what() const throw() override
	{
		return "Failure while saving helmet!";
	}
};


class bad_shield_save : public std::exception
{
public:
	virtual const char* what() const throw() override
	{
		return "Failure while saving shield!";
	}
};


class bad_ammo_load : public std::exception
{
public:
	virtual const char* what() const throw() override
	{
		return "Failure while loading ammo!";
	}
};


class bad_helmet_load : public std::exception
{
public:
	virtual const char* what() const throw() override
	{
		return "Failure while loading helmet!";
	}
};


class bad_shield_load : public std::exception
{
public:
	virtual const char* what() const throw() override
	{
		return "Failure while loading shield!";
	}
};
