from cvm._ffi.base import register_error, CVMError


@register_error
class InternalError(CVMError):
    """Internal error in the system.

    Examples
    --------
    .. code :: c++

        // Example code C++
        LOG(FATAL)

    """
    def __init__(self, msg):
        # Patch up additional hint message.
        if "CVM hint:" not in msg:
            msg += (
                "\nCVM hint: You hit an internal error."
                + "Please open a issue on https://github.com/robot-nano/CVM to report it."
            )
        super(InternalError, self).__init__(msg)


register_error("ValueError", ValueError)
register_error("TypeError", TypeError)
register_error("AttributeError", AttributeError)
register_error("KeyError", KeyError)
register_error("IndexError", IndexError)


@register_error
class OpError(CVMError):
    """Base class of call operator errors in frontends."""


@register_error
class OpNotImplemented(OpError, NotImplementedError):
    """Operator is not implemented.

    Examples
    --------
    .. code:: python

        raise OpNotImplemented(
            "Operator {} is not supported in {} fronted".format(
                missing_op, frontend_name))
    """


@register_error
class OpAttributeRequired(OpError, AttributeError):
    """Required attribute is not found.

    Examples
    --------
    .. code:: python

        raise OpAttributeRequired(
            "Required attribute {} not found in operator {}".format(
                attr_name, op_name))
    """


@register_error
class OpAttributeInvalid(OpError, AttributeError):
    """Attribute value is invalid when taking in a frontend operator.

    Examples
    --------
    .. code:: python

        raise OpAttributeInvalid(
            "Value {} in attribute {} of operator {} is not valid".format(
                value, attr_name, op_name)
    """


@register_error
class OpAttributeUnImplemented(OpError, NotImplementedError):
    """Attribute is not supported in a certain frontend.

    Examples
    --------
    .. code:: python

        raise OpAttributeUnImplemented(
            "Attribute {} is not supported in operator {}".format(
                attr_name, op_name))
    """


@register_error
class DiagnosticError(CVMError):
    """Error diagnostics were reported during the execution of a pass.

    See the configured diagnostic renderer for detailed error information.
    """
