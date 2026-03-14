#pragma once

#include <QMetaType>

namespace ShirohaChat
{

/// @brief Error codes for SendMessageUseCase
struct SendMessageErrors
{
    Q_GADGET

  public:
    enum class Code
    {
        None,
        RecipientMissing,
        GroupNotSynced,
        AuthFailed,
        PayloadTooLarge,
        NetworkError,
        Timeout
    };
    Q_ENUM(Code)
};
using SendMessageError = SendMessageErrors::Code;

/// @brief Error codes for FriendRequestUseCase
struct FriendActionErrors
{
    Q_GADGET

  public:
    enum class Code
    {
        None,
        AlreadyFriends,
        RequestNotFound,
        SelfRequest,
        NetworkError
    };
    Q_ENUM(Code)
};
using FriendActionError = FriendActionErrors::Code;

/// @brief Error codes for GroupManagementUseCase
struct GroupActionErrors
{
    Q_GADGET

  public:
    enum class Code
    {
        None,
        GroupNotFound,
        MemberLimitReached,
        NotMember,
        NotCreator,
        AlreadyMember,
        NetworkError
    };
    Q_ENUM(Code)
};
using GroupActionError = GroupActionErrors::Code;

/// @brief Error codes for AuthenticateUseCase
struct AuthenticateErrors
{
    Q_GADGET

  public:
    enum class Code
    {
        None,
        InvalidCredentials,
        NetworkError,
        ServerUnavailable,
        AlreadyAuthenticated
    };
    Q_ENUM(Code)
};
using AuthenticateError = AuthenticateErrors::Code;

} // namespace ShirohaChat

Q_DECLARE_METATYPE(ShirohaChat::SendMessageError)
Q_DECLARE_METATYPE(ShirohaChat::FriendActionError)
Q_DECLARE_METATYPE(ShirohaChat::GroupActionError)
Q_DECLARE_METATYPE(ShirohaChat::AuthenticateError)
