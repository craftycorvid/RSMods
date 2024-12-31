#pragma once

namespace D3DHelper {
	inline char dlldir[320];
	inline char* GetDirFile(char* name) {
		static char pldir[320];
		strcpy_s(pldir, dlldir);
		strcat_s(pldir, name);
		return pldir;
	}

	inline void Log(const char* fmt, ...)
	{
		if (!fmt)	return;

		char		text[4096];
		va_list		ap;
		va_start(ap, fmt);
		vsprintf_s(text, fmt, ap);
		va_end(ap);

		std::ofstream logfile;
		logfile.open(GetDirFile((PCHAR)"log.txt"), std::ios::app);
		if (logfile.is_open() && text)	logfile << text << std::endl;
		logfile.close();
	}
}

struct Mesh {
	UINT Stride;
	UINT PrimCount;
	UINT NumVertices;

	Mesh(UINT s, UINT p, UINT n) {
		Stride = s;
		PrimCount = p;
		NumVertices = n;
	}

	bool operator ==(const Mesh& meshB) const {
		return (Stride == meshB.Stride
			&& PrimCount == meshB.PrimCount
			&& NumVertices == meshB.NumVertices);
	}
};

struct ThiccMesh {
	UINT Stride;
	UINT PrimCount;
	UINT NumVertices;
	UINT StartIndex;
	UINT StartRegister;
	UINT PrimType;
	UINT DeclType;
	UINT VectorCount;
	UINT NumElements;

	ThiccMesh(UINT s, UINT p, UINT n, UINT si, UINT mSR, UINT pt, UINT dt, UINT mvc, UINT nm) {
		Stride = s;
		PrimCount = p;
		NumVertices = n;
		StartIndex = si;
		StartRegister = mSR;
		PrimType = pt;
		DeclType = dt;
		VectorCount = mvc;
		NumElements = nm;
	}

	std::string ToString() {
		std::string ret = "{ ";

		ret += std::to_string(Stride);
		ret += ",";
		ret += std::to_string(PrimCount);
		ret += ",";
		ret += std::to_string(NumVertices);
		ret += ",";
		ret += std::to_string(StartIndex);
		ret += ",";
		ret += std::to_string(StartRegister);
		ret += ",";
		ret += std::to_string(PrimType);
		ret += ",";
		ret += std::to_string(DeclType);
		ret += ",";
		ret += std::to_string(VectorCount);
		ret += ",";
		ret += std::to_string(NumElements);

		ret += " },";

		return ret;
	}

	bool operator ==(const ThiccMesh& meshB) const {
		return (Stride == meshB.Stride
			&& PrimCount == meshB.PrimCount
			&& NumVertices == meshB.NumVertices
			&& StartIndex == meshB.StartIndex
			&& StartRegister == meshB.StartRegister
			&& PrimType == meshB.PrimType
			&& DeclType == meshB.DeclType
			&& VectorCount == meshB.VectorCount
			&& NumElements == meshB.NumElements);
	}
};

inline bool IsExtraRemoved(std::vector<ThiccMesh> list, ThiccMesh mesh) {
	for (const auto& currentMesh : list)
		if (currentMesh == mesh)
			return true;

	return false;
}

inline bool IsToBeRemoved(std::vector<Mesh> list, Mesh mesh) {
	for (const auto& currentMesh : list)
		if (currentMesh == mesh)
			return true;

	return false;
}

/*--------------------------- GENERAL D3D -----------------------------*/


/*--------------------------- LOG -----------------------------*/

