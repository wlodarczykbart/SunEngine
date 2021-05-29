#include "StringUtil.h"
#include "GraphicsWindow.h"
#include "CommandBuffer.h"
#include "GraphicsContext.h"
#include "IDevice.h"

#include "spdlog/spdlog.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_decompose.hpp"

#include "View.h"

namespace SunEngine
{
	bool ParseValue(const String&key, const String& keyValue, String& value)
	{
		if (StrStartsWith(keyValue, key + '='))
		{
			value = keyValue.substr(key.length() + 1);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool View::CreateInfo::ParseConfigString(const String& str)
	{
		if (!str.length())
			return false;

		Vector<String> parts;
		StrSplit(str, parts, ',');

		width = 0;
		height = 0;
		visible = true;

		for (uint i = 0; i < parts.size(); i++)
		{
			String keyValue = StrTrimStart(StrTrimEnd(parts[i]));
			String value;

			if (ParseValue("width", keyValue, value))
			{
				width = StrToInt(value);
			}
			else if (ParseValue("height", keyValue, value))
			{
				height = StrToInt(value);
			}
			else if (ParseValue("visible", keyValue, value))
			{
				visible = StrToInt(value) == 0 ? false : true;
			}	
		}

		if (width == 0 || height == 0)
			return false;

		return true;
	}

	View::View(const String& name)
	{
		_name = name;
		_camMode = CM_FIRST_PERSON;
		_viewPos = glm::vec2(0.0f);
		_viewSize = glm::vec2(0.0f);
		_viewFocused = false;
		_needsResize = false;
		_mouseInsideView = false;

		_position = glm::vec3(0.0f);
		_rotation = glm::vec3(0.0f);
		_worldMtx = glm::mat4(1.0f);
		_viewMtx = glm::mat4(1.0f);

		_nearZ = 0.5f;
		_farZ = 500.0f;
		_aspectRatio = 1.0f;
		_fovAngle = 45.0f;
		_projMtx = glm::mat4(1.0f);

		for (uint i = 0; i < MOUSE_BUTTON_COUNT; i++)
			_mouseButtonStates[i] = false;

		_relativeMousePosition = glm::vec2(0);

		_renderToGraphicsWindow = false;
	}

	View::~View()
	{

	}

	void View::UpdateFirstPersonCamera(GraphicsWindow* pWindow, const GWEventData* pEvents, uint nEvents, float dt, float et)
	{
		if (_mouseButtonStates[MOUSE_RIGHT] && pWindow->KeyDown(KEY_RBUTTON))
		{
			int mx, my;
			pWindow->GetMousePosition(mx, my);

			glm::vec2 mousePos = glm::vec2((float)mx, (float)my);
			glm::vec2 delta = (mousePos - _mousePositions[MOUSE_RIGHT]) * 0.1f;

			_rotation.y += delta.x;
			_rotation.x += delta.y;

			_mousePositions[MOUSE_RIGHT] = mousePos;
		}

		glm::vec3 right = glm::normalize(glm::vec3(_worldMtx[0]));
		glm::vec3 forward = glm::normalize(glm::vec3(_worldMtx[2]));
		float speed = 0.1f;

		if (pWindow->KeyDown(KEY_W))
		{
			_position -= forward * speed;
		}
		if (pWindow->KeyDown(KEY_S))
		{
			_position += forward * speed;
		}
		if (pWindow->KeyDown(KEY_A))
		{
			_position -= right * speed;
		}
		if (pWindow->KeyDown(KEY_D))
		{
			_position += right * speed;
		}
		if (pWindow->KeyDown(KEY_R))
		{
			_position += glm::vec3(0,1,0) * speed;
		}
		if (pWindow->KeyDown(KEY_F))
		{
			_position -= glm::vec3(0,1,0) * speed;
		}

		_nearZ = 0.5f;
		_farZ = 500.0f;
		_aspectRatio = (float)_target.Width() / _target.Height();
		_fovAngle = 45.0f;
		_projMtx = glm::perspective(glm::radians(_fovAngle), _aspectRatio, _nearZ, _farZ);
		
		glm::mat4 mtxIden = glm::mat4(1.0f);
		glm::mat4 rotX = glm::rotate(mtxIden, glm::radians(_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 rotY = glm::rotate(mtxIden, glm::radians(_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 transMtx = glm::translate(mtxIden, _position);

		//should be equal to doing inverse of regular world transformation
		_worldMtx = transMtx * rotY * rotX;
		_viewMtx = glm::inverse(_worldMtx);
	}

	void View::GetTransform(glm::vec3& position, glm::quat& orientation) const
	{
		//TODO FIX SOMETIMES NOT WORKING
		//glm::vec3 scale, skew;
		//glm::vec4 perspective;
		//position = glm::vec3(0.0f);
		//orientation = glm::quat();
		//glm::decompose(_worldMtx, scale, orientation, position, skew, perspective);
		//orientation = glm::conjugate(orientation);

		position = _position;
		orientation = glm::angleAxis(glm::radians(_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::angleAxis(glm::radians(_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	bool View::Create(const CreateInfo& info)
	{
		RenderTarget::CreateInfo rtInfo = {};
		rtInfo.width = info.width;
		rtInfo.height = info.height;
		rtInfo.hasDepthBuffer = true;
		rtInfo.numTargets = 1;
		rtInfo.floatingPointColorBuffer = info.floatingPointColorBuffer;
		if (!_target.Create(rtInfo))
		{
			return false;
		}

		_viewSize = glm::vec2((float)info.width, (float)info.height);

		if (!OnCreate(info))
			return false;

		_info = info;

		return true;
	}

	void View::UpdateViewState(const glm::vec2& viewSize, const glm::vec2& viewPos, bool mouseInsideView, bool isFocused)
	{
		_needsResize = viewSize != _viewSize;
		_viewSize = viewSize;
		_viewPos = viewPos;
		_viewFocused = isFocused;
		_mouseInsideView = mouseInsideView;
	}

	bool View::Render(CommandBuffer* cmdBuffer)
	{
		if (!_target.Bind(cmdBuffer))
			return false;

		if (!_target.Unbind(cmdBuffer))
			return false;

		return true;
	}

	void View::Update(GraphicsWindow* pWindow, const GWEventData* pEvents, uint nEvents, float dt, float et)
	{
		if (_needsResize)
		{
			GraphicsContext::GetDevice()->WaitIdle();

			_info.width = (uint)_viewSize.x;
			_info.height = (uint)_viewSize.y;

			RenderTarget::CreateInfo rtInfo = {};
			rtInfo.width = (uint)_viewSize.x;
			rtInfo.height = (uint)_viewSize.y;
			rtInfo.hasDepthBuffer = true;
			rtInfo.numTargets = 1;
			rtInfo.floatingPointColorBuffer = _info.floatingPointColorBuffer;
			_target.Create(rtInfo);

			if (!OnResize(_info))
			{
				assert(false);
			}

			_needsResize = false;
		}

		for (uint i = 0; i < nEvents; i++)
		{
			if (pEvents[i].type == GWE_MOUSE_DOWN)
			{
				_mousePositions[pEvents[i].mouseButtonCode] = glm::vec2((float)pEvents[i].x, (float)pEvents[i].y);
				_mouseButtonStates[pEvents[i].mouseButtonCode] = true;
			}
			else if (pEvents[i].type == GWE_MOUSE_UP)
			{
				_mouseButtonStates[pEvents[i].mouseButtonCode] = false;
			}
			else if (pEvents[i].type == GWE_FOCUS_LOST)
			{
				for (uint j = 0; j < MOUSE_BUTTON_COUNT; j++)
				{
					_mouseButtonStates[j] = false;
				}

				//no update without focus due to calling async input functions
				return;
			}
		}

		int mx, my;
		pWindow->GetMousePosition(mx, my);
		if (_mouseInsideView)
		{
			_relativeMousePosition = glm::vec2(mx - _viewPos.x, my - _viewPos.y);
			_relativeMousePosition = glm::clamp(_relativeMousePosition, glm::vec2(0.0f, 0.0f), _viewSize);

			if (_viewFocused && _camMode == CM_FIRST_PERSON)
			{
				UpdateFirstPersonCamera(pWindow, pEvents, nEvents, dt, et);
			}
		}
	}

}