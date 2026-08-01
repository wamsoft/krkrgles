#ifndef __GL_TEXTURE_H__
#define __GL_TEXTURE_H__

#include "OpenGLHeader.h"

#include <functional>

class GLTexture {

	friend class tTJSNI_Offscreen;

protected:
	GLuint texture_id_;
	GLint format_;
	GLuint width_;
	GLuint height_;
	GLuint pbo_;

	GLenum stretchType_;
	GLenum wrapS_;
	GLenum wrapT_;
	bool hasMipmap_ = false;
public:
	GLTexture() : texture_id_(0), width_(0), height_(0), pbo_(0), format_(0), stretchType_(GL_LINEAR), wrapS_(GL_CLAMP_TO_EDGE), wrapT_(GL_CLAMP_TO_EDGE) {}
	GLTexture( GLuint w, GLuint h, const GLvoid* bits=0, GLint format=GL_RGBA ) : width_(w), height_(h), stretchType_(GL_LINEAR), wrapS_(GL_CLAMP_TO_EDGE), wrapT_(GL_CLAMP_TO_EDGE) {
		create( w, h, bits, format );
	}
	~GLTexture() {
		destory();
	}

	void create( GLuint w, GLuint h, const GLvoid* bits=0, GLint format=GL_RGBA );

	void destory();

	/**
	 * フィルタタイプに応じたミップマップテクスチャフィルタを返す
	 * GL_NEAREST_MIPMAP_LINEAR/GL_LINEAR_MIPMAP_LINEAR は使用していない
	 */
	static GLint getMipmapFilter( GLint filter ) {
		switch( filter ) {
		case GL_NEAREST:
			return GL_NEAREST_MIPMAP_NEAREST;
		case GL_LINEAR:
			return GL_LINEAR_MIPMAP_NEAREST;
		case GL_NEAREST_MIPMAP_NEAREST:
			return GL_NEAREST_MIPMAP_NEAREST;
		case GL_LINEAR_MIPMAP_NEAREST:
			return GL_LINEAR_MIPMAP_NEAREST;
		case GL_NEAREST_MIPMAP_LINEAR:
			return GL_NEAREST_MIPMAP_LINEAR;
		case GL_LINEAR_MIPMAP_LINEAR:
			return GL_LINEAR_MIPMAP_LINEAR;
		default:
			return GL_LINEAR_MIPMAP_NEAREST;
		}
	}

	static int getMaxTextureSize() {
		GLint maxTex;
		glGetIntegerv( GL_MAX_TEXTURE_SIZE, &maxTex );
		return maxTex;
	}
	GLuint width() const { return width_; }
	GLuint height() const { return height_; }
	GLuint id() const { return texture_id_; }
	GLint format() const { return format_; }
	GLint pbo() const { return pbo_; }

	GLenum stretchType() const { return stretchType_; }
	void setStretchType( GLenum s ) {
		if( texture_id_ && stretchType_ != s ) {
			glBindTexture( GL_TEXTURE_2D, texture_id_ );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, s );
			if( hasMipmap_ == false ) {
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, s );
			} else {
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, getMipmapFilter(s) );
			}
			glBindTexture( GL_TEXTURE_2D, 0 );
		}
		stretchType_ = s;
	}
	GLenum wrapS() const { return wrapS_; }
	void setWrapS( GLenum s ) {
		if( texture_id_ && wrapS_ != s ) {
			glBindTexture( GL_TEXTURE_2D, texture_id_ );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s );
			glBindTexture( GL_TEXTURE_2D, 0 );
		}
		wrapS_ = s;
	}
	GLenum wrapT() const { return wrapT_; }
	void setWrapT( GLenum s ) {
		if( texture_id_ && wrapT_ != s ) {
			glBindTexture( GL_TEXTURE_2D, texture_id_ );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s );
			glBindTexture( GL_TEXTURE_2D, 0 );
		}
		wrapT_ = s;
	}

    static void UpdateTexture(GLuint tex_id, GLuint pbo, int format, int x, int y, int w, int h, std::function<void(char *dest, int pitch)> updator);
    void UpdateTexture(int x, int y, int w, int h, std::function<void(char *dest, int pitch)> updator);
};

class GLTextureDrawer
{

public:
	GLTextureDrawer();
	~GLTextureDrawer();

	void Init();
	void Done();

	/**
	 * テクスチャ矩形の描画
	 * @param scissorBox 矩形クリップ (GL 座標系 x,y,w,h の4要素、nullptr で無効)
	 * @param stencilTest true なら stencil test を維持したまま描画する
	 *                    (ステンシル関数/参照値は呼び出し側で設定済みであること)
	 */
	void DrawTexture(GLTexture *tex, int scr_w, int scr_h, float position[], int tex_w=0, int tex_h=0, int opacity=255,
	                 const GLint *scissorBox=nullptr, bool stencilTest=false);

private:
	GLuint _shader_program;
	GLint _attr_position;
	GLint _attr_texCoord;
	GLint _unif_texture;
	GLint _unif_opacity;
};


#endif // __GL_TEXTURE_H__
