#pragma once

#include <exception>  // exception


class bad_record_info : public std::exception
{
public:
	virtual const char* what() const throw() override
	{
		return "Failed to get next record info!";
	}
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


class bad_ammo_load : public std::exception
{
public:
	virtual const char* what() const throw() override
	{
		return "Failure while loading ammo!";
	}
};
